#include <algorithm>
#include <array>
#include <sstream>
#include <ctime>

#include "NowayHmmReader.hh"
#include "WordSubwordDecoder.hh"

using namespace std;


WordSubwordDecoder::WordSubwordDecoder()
{
    m_class_iw = 0.0;
    m_word_iw = 0.0;
    m_subword_iw = 0.0;
    m_subword_lm_start_node = -1;
}


WordSubwordDecoder::~WordSubwordDecoder()
{
}


void
WordSubwordDecoder::read_lm(string lmfname)
{
    m_lm.read_arpa(lmfname);
    set_text_unit_id_ngram_symbol_mapping();
}


void
WordSubwordDecoder::set_text_unit_id_ngram_symbol_mapping()
{
    m_text_unit_id_to_ngram_symbol.resize(m_text_units.size(), -1);
    for (unsigned int i=0; i<m_text_units.size(); i++)
        m_text_unit_id_to_ngram_symbol[i] = m_lm.vocabulary_lookup[m_text_units[i]];
}


void
WordSubwordDecoder::read_class_lm(
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
}


int
WordSubwordDecoder::read_class_memberships(
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
        float prob;
        ss >> word >> clss >> prob;
        class_memberships[word] = make_pair(clss, prob);
        max_class = max(max_class, clss);
    }
    return max_class+1;
}


void
WordSubwordDecoder::read_subword_lm(
    string ngramfname,
    string segfname)
{
    cerr << "Reading subword n-gram: " << ngramfname << endl;
    m_subword_lm.read_arpa(ngramfname);

    cerr << "Reading word segmentations: " << segfname << endl;
    ifstream segf(segfname);
    if (!segf) throw string("Problem opening word segmentations.");

    map<string, vector<string> > word_segs;
    string line;
    int linei = 1;
    while (getline(segf, line)) {
        if (line.length() == 0) continue;
        string word;
        string subword;
        stringstream ss(line);
        ss >> word;
        string concatenated;
        vector<string> sw_tokens;
        while (ss >> subword) {
            sw_tokens.push_back(subword);
            concatenated += subword;
        }
        //if (concatenated != word || sw_tokens.size() == 0)
        //    throw "Erroneous segmentation: " + concatenated;
        word_segs[word] = sw_tokens;
        linei++;
    }

    bool wb_symbol_in_use =
        (m_subword_lm.vocabulary_lookup.find("<w>") != m_subword_lm.vocabulary_lookup.end());
    m_word_id_to_subword_ngram_symbols.resize(m_text_units.size());
    for (unsigned int i=0; i<m_text_units.size(); i++) {
        string wrd(m_text_units[i]);
        vector<int> sw_symbols;

        if (wrd[0] == '<') {
            sw_symbols.push_back(m_subword_lm.vocabulary_lookup[wrd]);
        }
        else {
            auto swsegit = word_segs.find(wrd);
            if (swsegit == word_segs.end())
                throw "Segmentation not found for word: " + wrd;
            for (auto swit = swsegit->second.begin(); swit != swsegit->second.end(); ++swit)
                sw_symbols.push_back(m_subword_lm.vocabulary_lookup[*swit]);
            if (wb_symbol_in_use)
                sw_symbols.push_back(m_subword_lm.vocabulary_lookup["<w>"]);
        }
        m_word_id_to_subword_ngram_symbols[i] = sw_symbols;
    }

    word_segs.clear();

    m_subword_lm_start_node = m_subword_lm.sentence_start_node;
    if (wb_symbol_in_use)
        m_subword_lm_start_node
            = m_subword_lm.advance(m_subword_lm_start_node, m_subword_lm.vocabulary_lookup["<w>"]);
}


void
WordSubwordDecoder::print_config(ostream &outf)
{
    Decoder::print_config(outf);
    outf << "interpolation weight (word lm): " << exp(m_word_iw) << endl;
    outf << "interpolation weight (class lm): " << exp(m_class_iw) << endl;
    outf << "interpolation weight (subword lm): " << exp(m_subword_iw) << endl;
}


WordSubwordRecognition::WordSubwordRecognition(WordSubwordDecoder &decoder)
    : Recognition::Recognition(decoder)
{
    wswd = static_cast<WordSubwordDecoder*>(d);
    m_histogram_bin_limit = 0;
    m_word_history_leafs.clear();
    m_raw_tokens.clear();
    m_raw_tokens.reserve(1000000);
    m_recombined_tokens.clear();
    m_recombined_tokens.resize(d->m_nodes.size());
    m_best_node_scores.resize(d->m_nodes.size(), -1e20);
    m_active_nodes.clear();
    m_active_histories.clear();
    WSWToken tok;
    tok.d = &decoder;
    tok.lm_node = wswd->m_lm.sentence_start_node;
    tok.class_lm_node = wswd->m_class_lm.sentence_start_node;
    tok.subword_lm_node = wswd->m_subword_lm_start_node;
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
WordSubwordRecognition::reset_frame_variables()
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
WordSubwordRecognition::propagate_tokens()
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
WordSubwordRecognition::prune_tokens(
    bool collect_active_histories,
    bool write_nbest)
{
    vector<WSWToken> pruned_tokens;
    pruned_tokens.reserve(50000);

    // Global beam pruning
    // Node beam pruning
    // Word end beam pruning
    float current_glob_beam = m_best_log_prob - m_global_beam;
    float current_word_end_beam = m_best_word_end_prob - m_word_end_beam;
    for (unsigned int i=0; i<m_raw_tokens.size(); i++) {
        WSWToken &tok = m_raw_tokens[i];

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
        map<pair<int, int>, WSWToken> &node_tokens = m_recombined_tokens[tit->node_idx];
        auto bntit = node_tokens.find(make_pair(tit->lm_node, tit->class_lm_node));
        if (bntit != node_tokens.end()) {
            if (tit->total_log_prob > bntit->second.total_log_prob) {
                histogram[bntit->second.histogram_bin]--;
                histogram[tit->histogram_bin]++;

                if (write_nbest // && bntit->second.history != tit->history
                    && bntit->second.history != tit->history
                    && bntit->second.history != tit->history->previous
                    && bntit->second.history->previous != tit->history
                    && (!bntit->second.history->previous ||
                        bntit->second.history->previous->previous != tit->history))
                {
                    array<float,3> weights = {
                        bntit->second.total_log_prob - tit->total_log_prob,
                        bntit->second.am_log_prob - tit->am_log_prob,
                        bntit->second.lm_log_prob - tit->lm_log_prob
                    };
                    WordHistory *previousBestHistory = bntit->second.history;

                    auto blit = tit->history->recombination_links.find(previousBestHistory);
                    if (blit == tit->history->recombination_links.end()) {
                        tit->history->recombination_links[previousBestHistory] = weights;
                        m_num_recombination_links++;
                    } else if (weights[0] > blit->second.at(0)) {
                        tit->history->recombination_links[previousBestHistory] = weights;
                        m_num_recombination_link_updated++;
                    } else
                        m_num_recombination_link_not_updated++;
                }

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
WordSubwordRecognition::move_token_to_node(WSWToken token,
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
            token.lm_node = wswd->m_lm.sentence_start_node;
            token.class_lm_node = wswd->m_class_lm.sentence_start_node;
            token.subword_lm_node = wswd->m_subword_lm_start_node;
            token.last_word_id = m_sentence_begin_symbol_idx;
        }
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
        move_token_to_node(token, ait->target_node, ait->log_prob, ait->update_lookahead);
}


bool
WordSubwordRecognition::update_lm_prob(WSWToken &token, int word_id)
{
    static double ln_to_log10 = 1.0/log(10.0);

    double class_lm_prob = class_lm_score(token, word_id);
    if (class_lm_prob == MIN_LOG_PROB) return false;
    class_lm_prob += wswd->m_class_iw;

    double word_lm_prob = 0.0;
    token.lm_node = wswd->m_lm.score(token.lm_node, wswd->m_text_unit_id_to_ngram_symbol[word_id], word_lm_prob);
    word_lm_prob += wswd->m_word_iw;

    double subword_lm_prob = 0.0;
    vector<int> &subword_ids = wswd->m_word_id_to_subword_ngram_symbols[word_id];
    for (auto swit=subword_ids.begin(); swit != subword_ids.end(); ++swit)
        token.subword_lm_node = wswd->m_subword_lm.score(token.subword_lm_node, *swit, subword_lm_prob);
    subword_lm_prob += wswd->m_subword_iw;

    double interpolated_lp = add_log_domain_probs(word_lm_prob, class_lm_prob);
    interpolated_lp = add_log_domain_probs(interpolated_lp, subword_lm_prob);
    token.lm_log_prob += ln_to_log10 * interpolated_lp;

    return true;
}


double
WordSubwordRecognition::class_lm_score(WSWToken &token, int word_id)
{
    double ngram_prob = 0.0;
    if (word_id == m_sentence_end_symbol_idx) {
        token.class_lm_node =
            wswd->m_class_lm.score(token.class_lm_node,
                                   wswd->m_class_lm.sentence_end_symbol_idx,
                                   ngram_prob);
        return ngram_prob;
    }
    else {
        if (wswd->m_class_membership_lookup[word_id].second == MIN_LOG_PROB) return MIN_LOG_PROB;
        double membership_prob = wswd->m_class_membership_lookup[word_id].second;
        token.class_lm_node =
            wswd->m_class_lm.score(token.class_lm_node,
                                   wswd->m_class_intmap[wswd->m_class_membership_lookup[word_id].first],
                                   ngram_prob);
        return membership_prob + ngram_prob;
    }
}


void
WordSubwordRecognition::get_tokens(vector<Token*> &tokens)
{
    tokens.clear();
    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit) {
        map<pair<int,int>, WSWToken> & node_tokens = m_recombined_tokens[*nit];
        for (auto tit = node_tokens.begin(); tit != node_tokens.end(); ++tit)
            tokens.push_back(&(tit->second));
    }
}


void
WordSubwordRecognition::add_sentence_ends(vector<Token*> &tokens)
{
    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        WSWToken *token = static_cast<WSWToken*>(*tit);
        if (token->lm_node == wswd->m_lm.sentence_start_node) continue;
        update_lm_prob(*token, m_sentence_end_symbol_idx);
        token->update_total_log_prob();
        advance_in_word_history(token, m_sentence_end_symbol_idx);
    }
}
