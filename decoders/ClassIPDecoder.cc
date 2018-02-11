#include <algorithm>
#include <sstream>
#include <ctime>

#include "NowayHmmReader.hh"
#include "ClassIPDecoder.hh"

using namespace std;


ClassIPDecoder::ClassIPDecoder()
{
    m_iw = 0.0;
    m_class_iw = 0.0;
    m_word_iw = 0.0;
}


ClassIPDecoder::~ClassIPDecoder()
{
}


void
ClassIPDecoder::read_lm(string lmfname)
{
    m_lm.read_arpa(lmfname);
    set_text_unit_id_ngram_symbol_mapping();
}


void
ClassIPDecoder::set_text_unit_id_ngram_symbol_mapping()
{
    m_text_unit_id_to_ngram_symbol.resize(m_text_units.size(), -1);
    for (unsigned int i=0; i<m_text_units.size(); i++) {
        string tmp(m_text_units[i]);
        m_text_unit_id_to_ngram_symbol[i] = m_lm.vocabulary_lookup[tmp];
    }
}


void
ClassIPDecoder::read_class_lm(
        string ngramfname,
        string classmfname)
{
    cerr << "Reading class n-gram: " << ngramfname << endl;
    m_class_lm.read_arpa(ngramfname);
    cerr << "Reading class membership probs.." << classmfname << endl;
    int num_classes = read_class_memberships(classmfname, m_class_memberships);

    m_class_membership_lookup.resize(m_text_units.size(), make_pair(-1,MIN_LOG_PROB));
    for (auto wpit=m_class_memberships.begin(); wpit!= m_class_memberships.end(); ++wpit) {
        if (wpit->first == "<unk>") continue;
        if (m_text_unit_map.find(wpit->first) == m_text_unit_map.end()) continue;
        int word_idx = m_text_unit_map.at(wpit->first);
        m_class_membership_lookup[word_idx] = wpit->second;
    }

    m_class_intmap.resize(num_classes);
    for (int i=0; i<(int)m_class_intmap.size(); i++)
        m_class_intmap[i] = m_class_lm.vocabulary_lookup[int2str(i)];

    cerr << "Weight for word n-gram: " << m_iw << endl;
    cerr << "Weight for class n-gram: " << 1.0-m_iw << endl;
    m_word_iw = log(m_iw);
    m_class_iw = log(1.0-m_iw);
}


int
ClassIPDecoder::read_class_memberships(
        string fname,
        map<string, pair<int, float> > &class_memberships)
{
    SimpleFileInput wcf(fname);

    string line;
    int max_class = 0;
    while (wcf.getline(line)) {
        stringstream ss(line);
        string word;
        int clss;
        double prob;
        ss >> word >> clss >> prob;
        class_memberships[word] = make_pair(clss, prob);
        max_class = max(max_class, clss);
    }
    return max_class+1;
}


ClassIPRecognition::ClassIPRecognition(ClassIPDecoder &decoder)
    : Recognition::Recognition(decoder)
{
    cid = static_cast<ClassIPDecoder*>(d);
    m_histogram_bin_limit = 0;
    m_word_history_leafs.clear();
    m_raw_tokens.clear();
    m_raw_tokens.reserve(1000000);
    m_recombined_tokens.clear();
    m_recombined_tokens.resize(d->m_nodes.size());
    m_best_node_scores.resize(d->m_nodes.size(), -1e20);
    m_active_nodes.clear();
    m_active_histories.clear();
    ClassIPToken tok;
    tok.d = &decoder;
    tok.lm_node = cid->m_lm.sentence_start_node;
    tok.class_lm_node = cid->m_class_lm.sentence_start_node;
    tok.last_word_id = m_sentence_begin_symbol_idx;
    tok.history = new WordHistory();
    tok.history->word_id = m_sentence_begin_symbol_idx;
    m_history_root = tok.history;
    tok.node_idx = d->m_decode_start_node;
    m_active_nodes.insert(d->m_decode_start_node);
    m_word_history_leafs.insert(tok.history);
    m_active_histories.insert(tok.history);
    m_recombined_tokens[d->m_decode_start_node][make_pair(tok.lm_node, tok.class_lm_node)] = tok;
    m_total_token_count = 0;
}


void
ClassIPRecognition::reset_frame_variables()
{
    m_token_count = 0;
    m_best_log_prob = -1e20;
    m_best_word_end_prob = -1e20;
    m_global_beam_pruned_count = 0;
    m_max_state_duration_pruned_count = 0;
    m_word_end_beam_pruned_count = 0;
    m_node_beam_pruned_count = 0;
    m_histogram_pruned_count = 0;
    m_dropped_count = 0;
    m_token_count_after_pruning = 0;
    m_active_histories.clear();
    fill(m_best_node_scores.begin(), m_best_node_scores.end(), -1e20);
}


void
ClassIPRecognition::propagate_tokens()
{
    int node_count = 0;
    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit) {
        Decoder::Node &node = d->m_nodes[*nit];
        for (auto tit = m_recombined_tokens[*nit].begin(); tit != m_recombined_tokens[*nit].end(); ++tit) {

            if (tit->second.histogram_bin < m_histogram_bin_limit) {
                m_histogram_pruned_count++;
                continue;
            }

            m_token_count++;
            tit->second.word_end = false;

            for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
                move_token_to_node(tit->second, ait->target_node, ait->log_prob, ait->update_lookahead);
        }
        node_count++;
    }

    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit)
        m_recombined_tokens[*nit].clear();

    if (m_stats)
        cerr << "token count before propagation: " << m_token_count << endl;
}


void
ClassIPRecognition::prune_tokens(bool collect_active_histories)
{
    vector<ClassIPToken> pruned_tokens;
    pruned_tokens.reserve(50000);

    // Global beam pruning
    // Node beam pruning
    // Word end beam pruning
    float current_glob_beam = m_best_log_prob - m_global_beam;
    float current_word_end_beam = m_best_word_end_prob - m_word_end_beam;
    for (unsigned int i=0; i<m_raw_tokens.size(); i++) {
        ClassIPToken &tok = m_raw_tokens[i];

        if (tok.total_log_prob < current_glob_beam) {
            m_global_beam_pruned_count++;
            continue;
        }

        if (tok.total_log_prob < (m_best_node_scores[tok.node_idx] - m_node_beam)) {
            m_node_beam_pruned_count++;
            continue;
        }

        if (tok.word_end) {
            float prob_wo_la = tok.total_log_prob - d->m_lm_scale * tok.lookahead_log_prob;
            if (prob_wo_la < current_word_end_beam) {
                m_word_end_beam_pruned_count++;
                continue;
            }
        }

        tok.histogram_bin = (int) round((float)(HISTOGRAM_BIN_COUNT-1) * (tok.total_log_prob-current_glob_beam)/m_global_beam);
        pruned_tokens.push_back(tok);
    }
    m_raw_tokens.clear();

    // Recombine node/ngram state hypotheses
    m_active_nodes.clear();
    vector<int> histogram(HISTOGRAM_BIN_COUNT, 0);
    for (auto tit = pruned_tokens.begin(); tit != pruned_tokens.end(); tit++) {
        map<pair<int, int>, ClassIPToken> &node_tokens = m_recombined_tokens[tit->node_idx];
        auto bntit = node_tokens.find(make_pair(tit->lm_node, tit->class_lm_node));
        if (bntit != node_tokens.end()) {
            if (tit->total_log_prob > bntit->second.total_log_prob) {
                histogram[bntit->second.histogram_bin]--;
                histogram[tit->histogram_bin]++;
                bntit->second = *tit;
            }
            m_dropped_count++;
        }
        else {
            node_tokens[make_pair(tit->lm_node, tit->class_lm_node)] = *tit;
            m_active_nodes.insert(tit->node_idx);
            histogram[tit->histogram_bin]++;
            m_token_count_after_pruning++;
        }
    }

    if (collect_active_histories)
        for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit)
            for (auto tit = m_recombined_tokens[*nit].begin(); tit != m_recombined_tokens[*nit].end(); ++tit)
                m_active_histories.insert(tit->second.history);

    m_histogram_bin_limit = 0;
    int histogram_tok_count = 0;
    for (int i=HISTOGRAM_BIN_COUNT-1; i>= 0; i--) {
        histogram_tok_count += histogram[i];
        if (histogram_tok_count > d->m_token_limit) {
            m_histogram_bin_limit = i;
            break;
        }
    }

    if (m_stats) cerr << "token count after pruning: " << m_token_count_after_pruning << endl;
    if (m_stats) cerr << "histogram index: " << m_histogram_bin_limit << endl;
}


void
ClassIPRecognition::move_token_to_node(ClassIPToken token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead)
{
    token.am_log_prob += m_transition_scale * transition_score;

    Decoder::Node &node = d->m_nodes[node_idx];

    if (token.node_idx == node_idx) {
        token.dur++;
        if (token.dur > m_max_state_duration && node.hmm_state > m_last_sil_idx) {
            m_max_state_duration_pruned_count++;
            return;
        }
    }
    else {
        // Apply duration model for previous state if moved out from a hmm state
        if (m_duration_model_in_use && d->m_nodes[token.node_idx].hmm_state != -1)
            token.apply_duration_model();
        token.node_idx = node_idx;
        token.dur = 1;
    }

    // HMM node
    if (node.hmm_state != -1) {

        if (update_lookahead)
            token.update_lookahead_prob(d->m_la->get_lookahead_score(node_idx, token.last_word_id));

        token.am_log_prob += m_acoustics->log_prob(node.hmm_state);

        token.update_total_log_prob();
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }

        m_best_log_prob = max(m_best_log_prob, token.total_log_prob);
        if (token.word_end) {
            float lp_wo_la = token.total_log_prob - d->m_lm_scale * token.lookahead_log_prob;
            m_best_word_end_prob = max(m_best_word_end_prob, lp_wo_la);
        }
        m_best_node_scores[node_idx] = max(m_best_node_scores[node_idx], token.total_log_prob);
        m_raw_tokens.push_back(token);
        return;
    }

    // LM node
    // Update LM score
    // Update history
    if (node.word_id != -1) {

        if (!update_lm_prob(token, node.word_id)) return;
        token.last_word_id = node.word_id;

        if (update_lookahead) {
            if (node.word_id == m_sentence_end_symbol_idx)
                token.update_lookahead_prob(d->m_la->get_lookahead_score(node_idx, m_sentence_begin_symbol_idx));
            else
                token.update_lookahead_prob(d->m_la->get_lookahead_score(node_idx, token.last_word_id));
        }

        token.update_total_log_prob();
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }

        advance_in_word_history(&token, node.word_id);
        token.word_end = true;

        if (node.word_id == m_sentence_end_symbol_idx) {
            token.lm_node = cid->m_lm.sentence_start_node;
            token.class_lm_node = cid->m_class_lm.sentence_start_node;
            token.last_word_id = m_sentence_begin_symbol_idx;
        }
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
        move_token_to_node(token, ait->target_node, ait->log_prob, ait->update_lookahead);
}


bool
ClassIPRecognition::update_lm_prob(ClassIPToken &token, int word_id)
{
    double class_lm_prob = class_lm_score(token, word_id);
    if (class_lm_prob == MIN_LOG_PROB) return false;
    class_lm_prob += cid->m_class_iw;

    double word_lm_prob = 0.0;
    token.lm_node = cid->m_lm.score(token.lm_node, cid->m_text_unit_id_to_ngram_symbol[word_id], word_lm_prob);
    word_lm_prob += cid->m_word_iw;

    static double ln_to_log10 = 1.0/log(10.0);
    token.lm_log_prob += ln_to_log10 * add_log_domain_probs(word_lm_prob, class_lm_prob);

    return true;
}


double
ClassIPRecognition::class_lm_score(ClassIPToken &token, int word_id)
{
    double ngram_prob = 0.0;
    if (word_id == m_sentence_end_symbol_idx) {
        token.class_lm_node =
                cid->m_class_lm.score(token.class_lm_node,
                                      cid->m_class_lm.sentence_end_symbol_idx,
                                      ngram_prob);
        return ngram_prob;
    }
    else {
        if (cid->m_class_membership_lookup[word_id].second == MIN_LOG_PROB) return MIN_LOG_PROB;
        double membership_prob = cid->m_class_membership_lookup[word_id].second;
        token.class_lm_node =
                cid->m_class_lm.score(token.class_lm_node,
                                      cid->m_class_intmap[cid->m_class_membership_lookup[word_id].first],
                                      ngram_prob);
        return membership_prob + ngram_prob;
    }

}


void
ClassIPRecognition::get_tokens(vector<Token*> &tokens)
{
    tokens.clear();
    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit) {
        map<pair<int,int>, ClassIPToken> &node_tokens = m_recombined_tokens[*nit];
        for (auto tit = node_tokens.begin(); tit != node_tokens.end(); ++tit)
            tokens.push_back(&(tit->second));
    }
}


void
ClassIPRecognition::add_sentence_ends(vector<Token*> &tokens)
{
    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        ClassIPToken *token = static_cast<ClassIPToken*>(*tit);
        if (token->lm_node == cid->m_lm.sentence_start_node) continue;
        update_lm_prob(*token, m_sentence_end_symbol_idx);
        token->update_total_log_prob();
        advance_in_word_history(token, m_sentence_end_symbol_idx);
    }
}


string
ClassIPRecognition::get_best_word_history()
{
    vector<Token*> tokens;
    get_tokens(tokens);
    return get_word_history(get_best_token(tokens)->history);
}


string
ClassIPRecognition::get_word_history(WordHistory *history)
{
    string result;
    vector<int> text_units;
    while (true) {
        text_units.push_back(history->word_id);
        if (history->previous == nullptr) break;
        history = history->previous;
    }

    for (auto swit = text_units.rbegin(); swit != text_units.rend(); ++swit)
        if (*swit >= 0) result += " " + m_text_units->at(*swit);

    return result;
}
