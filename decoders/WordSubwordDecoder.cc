#include <algorithm>
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
    for (unsigned int i=0; i<m_text_units.size(); i++) {
        string tmp(m_text_units[i]);
        m_text_unit_id_to_ngram_symbol[i] = m_lm.vocabulary_lookup[tmp];
    }
}


void
WordSubwordDecoder::read_class_lm(string ngramfname,
                       string classmfname)
{
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
    m_class_membership_lookup[m_text_unit_map.at("</s>")] = m_class_membership_lookup[m_text_unit_map.at("<s>")];

    m_class_intmap.resize(num_classes);
    for (int i=0; i<(int)m_class_intmap.size(); i++)
        m_class_intmap[i] = m_class_lm.vocabulary_lookup[int2str(i)];
}


int
WordSubwordDecoder::read_class_memberships(string fname,
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
WordSubwordDecoder::read_subword_lm(string ngramfname,
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
        if (concatenated != word || sw_tokens.size() == 0)
            throw "Erroneous segmentation: " + concatenated;
        word_segs[word] = sw_tokens;
        linei++;
    }

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
            sw_symbols.push_back(m_subword_lm.vocabulary_lookup["<w>"]);
        }
        m_word_id_to_subword_ngram_symbols[i] = sw_symbols;
    }

    word_segs.clear();

    m_subword_lm_start_node = m_subword_lm.advance(m_subword_lm.root_node, m_subword_lm.vocabulary_lookup["<s>"]);
    m_subword_lm_start_node = m_subword_lm.advance(m_subword_lm_start_node, m_subword_lm.vocabulary_lookup["<w>"]);
}


void
WordSubwordDecoder::recognize_lna_file(string lnafname,
                            ostream &outf,
                            int *frame_count,
                            double *seconds,
                            double *log_prob,
                            double *am_prob,
                            double *lm_prob,
                            double *total_token_count)
{
    m_lna_reader.open_file(lnafname, 1024);
    m_acoustics = &m_lna_reader;
    initialize();

    time_t start_time, end_time;
    time(&start_time);
    m_frame_idx = 0;
    while (m_lna_reader.go_to(m_frame_idx)) {

        reset_frame_variables();
        propagate_tokens();

        if (m_frame_idx % m_history_clean_frame_interval == 0) {
            prune_tokens(true);
            prune_word_history();
            //print_certain_word_history();
        }
        else prune_tokens(false);

        if (m_stats) {
            cerr << endl << "recognized frame: " << m_frame_idx << endl;
            cerr << "current global beam: " << m_global_beam << endl;
            cerr << "tokens pruned by global beam: " << m_global_beam_pruned_count << endl;
            cerr << "tokens dropped by max assumption: " << m_dropped_count << endl;
            cerr << "tokens pruned by word end beam: " << m_word_end_beam_pruned_count << endl;
            cerr << "tokens pruned by node beam: " << m_node_beam_pruned_count << endl;
            cerr << "tokens pruned by histogram token limit: " << m_histogram_pruned_count << endl;
            cerr << "tokens pruned by max state duration: " << m_max_state_duration_pruned_count << endl;
            cerr << "best log probability: " << m_best_log_prob << endl;
            cerr << "number of active nodes: " << m_active_nodes.size() << endl;
            print_best_word_history(cerr);
        }

        m_total_token_count += double(m_token_count);
        m_frame_idx++;
    }
    time(&end_time);

    vector<Token> tokens;
    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit) {
        map<pair<int, int>, Token> &node_tokens = m_recombined_tokens[*nit];
        for (auto tit = node_tokens.begin(); tit != node_tokens.end(); ++tit)
            tokens.push_back(tit->second);
    }

    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        Token &tok = *tit;
        if (m_duration_model_in_use && tok.dur > 1) apply_duration_model(tok, tok.node_idx);
        update_lookahead_prob(tok, 0.0);
        update_total_log_prob(tok);
    }

    Token *best_token = nullptr;
    best_token = get_best_end_token(tokens);
    if (best_token == nullptr) {
        if (m_force_sentence_end) add_sentence_ends(tokens);
        best_token = get_best_token(tokens);
    }
    //add_sentence_ends(tokens);
    //Token *best_token = get_best_token(tokens);

    outf << lnafname << ":";
    print_word_history(best_token->history, outf, false);

    clear_word_history();
    m_lna_reader.close();

    if (frame_count != nullptr) *frame_count = m_frame_idx;
    if (seconds != nullptr) *seconds = difftime(end_time, start_time);
    if (log_prob != nullptr) *log_prob = best_token->total_log_prob;
    if (am_prob != nullptr) *am_prob = best_token->am_log_prob;
    if (lm_prob != nullptr) *lm_prob = best_token->lm_log_prob;
    if (total_token_count != nullptr) *total_token_count = m_total_token_count;
}


void
WordSubwordDecoder::initialize()
{
    m_histogram_bin_limit = 0;
    m_word_history_leafs.clear();
    m_raw_tokens.clear();
    m_raw_tokens.reserve(1000000);
    m_recombined_tokens.clear();
    m_recombined_tokens.resize(m_nodes.size());
    m_best_node_scores.resize(m_nodes.size(), -1e20);
    m_active_nodes.clear();
    m_active_histories.clear();
    Token tok;
    tok.lm_node = m_lm.sentence_start_node;
    tok.class_lm_node = m_class_lm.sentence_start_node;
    tok.subword_lm_node = m_subword_lm_start_node;
    tok.last_word_id = m_sentence_begin_symbol_idx;
    m_ngram_state_sentence_begin = tok.lm_node;
    tok.history = new WordHistory();
    tok.history->word_id = m_sentence_begin_symbol_idx;
    m_history_root = tok.history;
    tok.node_idx = m_decode_start_node;
    m_active_nodes.insert(m_decode_start_node);
    m_word_history_leafs.insert(tok.history);
    m_active_histories.insert(tok.history);
    m_recombined_tokens[m_decode_start_node][make_pair(tok.lm_node, tok.class_lm_node)] = tok;
    m_total_token_count = 0;
}


void
WordSubwordDecoder::reset_frame_variables()
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
WordSubwordDecoder::propagate_tokens()
{
    vector<int> sorted_active_nodes;
    active_nodes_sorted_by_best_lp(sorted_active_nodes);

    int node_count = 0;
    for (auto nit = sorted_active_nodes.begin(); nit != sorted_active_nodes.end(); ++nit) {
        Node &node = m_nodes[*nit];
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

    for (auto nit = sorted_active_nodes.begin(); nit != sorted_active_nodes.end(); ++nit)
        m_recombined_tokens[*nit].clear();

    if (m_stats)
        cerr << "token count before propagation: " << m_token_count << endl;
}


void
WordSubwordDecoder::prune_tokens(bool collect_active_histories)
{
    vector<Token> pruned_tokens;
    pruned_tokens.reserve(50000);

    // Global beam pruning
    // Node beam pruning
    // Word end beam pruning
    float current_glob_beam = m_best_log_prob - m_global_beam;
    float current_word_end_beam = m_best_word_end_prob - m_word_end_beam;
    for (unsigned int i=0; i<m_raw_tokens.size(); i++) {
        Token &tok = m_raw_tokens[i];

        if (tok.total_log_prob < current_glob_beam) {
            m_global_beam_pruned_count++;
            continue;
        }

        if (tok.total_log_prob < (m_best_node_scores[tok.node_idx] - m_node_beam)) {
            m_node_beam_pruned_count++;
            continue;
        }

        if (tok.word_end) {
            float prob_wo_la = tok.total_log_prob - m_lm_scale * tok.lookahead_log_prob;
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
        map<pair<int, int>, Token> &node_tokens = m_recombined_tokens[tit->node_idx];
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
        if (histogram_tok_count > m_token_limit) {
            m_histogram_bin_limit = i;
            break;
        }
    }

    if (m_stats) cerr << "token count after pruning: " << m_token_count_after_pruning << endl;
    if (m_stats) cerr << "histogram index: " << m_histogram_bin_limit << endl;
}


void
WordSubwordDecoder::move_token_to_node(Token token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead)
{
    token.am_log_prob += m_transition_scale * transition_score;

    Node &node = m_nodes[node_idx];

    if (token.node_idx == node_idx) {
        token.dur++;
        if (token.dur > m_max_state_duration && node.hmm_state > m_last_sil_idx) {
            m_max_state_duration_pruned_count++;
            return;
        }
    }
    else {
        // Apply duration model for previous state if moved out from a hmm state
        if (m_duration_model_in_use && m_nodes[token.node_idx].hmm_state != -1)
            apply_duration_model(token, token.node_idx);
        token.node_idx = node_idx;
        token.dur = 1;
    }

    // HMM node
    if (node.hmm_state != -1) {

        if (update_lookahead)
            update_lookahead_prob(token, m_la->get_lookahead_score(node_idx, token.last_word_id));

        token.am_log_prob += m_acoustics->log_prob(node.hmm_state);

        update_total_log_prob(token);
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }

        m_best_log_prob = max(m_best_log_prob, token.total_log_prob);
        if (token.word_end) {
            float lp_wo_la = token.total_log_prob - m_lm_scale * token.lookahead_log_prob;
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
            if (node.word_id == m_sentence_end_symbol_idx) {
                update_lookahead_prob(token, m_la->get_lookahead_score(node_idx, m_sentence_begin_symbol_idx));
            }
            else {
                update_lookahead_prob(token, m_la->get_lookahead_score(node_idx, token.last_word_id));
            }
        }

        update_total_log_prob(token);
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }

        advance_in_word_history(token, node.word_id);
        token.word_end = true;

        if (node.word_id == m_sentence_end_symbol_idx) {
            token.lm_node = m_ngram_state_sentence_begin;
            token.class_lm_node = m_class_lm.sentence_start_node;
            token.subword_lm_node = m_subword_lm_start_node;
            token.last_word_id = m_sentence_begin_symbol_idx;
        }
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
        move_token_to_node(token, ait->target_node, ait->log_prob, ait->update_lookahead);
}


WordSubwordDecoder::Token*
WordSubwordDecoder::get_best_token()
{
    Token *best_token = nullptr;

    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit) {
        map<pair<int, int>, Token> & node_tokens = m_recombined_tokens[*nit];
        for (auto tit = node_tokens.begin(); tit != node_tokens.end(); ++tit) {
            if (best_token == nullptr)
                best_token = &(tit->second);
            else if (tit->second.total_log_prob > best_token->total_log_prob)
                best_token = &(tit->second);
        }
    }

    return best_token;
}


WordSubwordDecoder::Token*
WordSubwordDecoder::get_best_token(vector<Token> &tokens)
{
    Token *best_token = nullptr;

    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        if (best_token == nullptr)
            best_token = &(*tit);
        else if (tit->total_log_prob > best_token->total_log_prob)
            best_token = &(*tit);
    }

    return best_token;
}


WordSubwordDecoder::Token*
WordSubwordDecoder::get_best_end_token(vector<Token> &tokens)
{
    Token *best_token = nullptr;

    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        //if (tit->lm_node != m_ngram_state_sentence_begin) continue;

        WordSubwordDecoder::Node &node = m_nodes[tit->node_idx];
        if (node.flags & NODE_SILENCE) {
            if (best_token == nullptr)
                best_token = &(*tit);
            else if (tit->total_log_prob > best_token->total_log_prob)
                best_token = &(*tit);
        }
    }

    return best_token;
}


void
WordSubwordDecoder::advance_in_word_history(Token &token, int word_id)
{
    auto next_history = token.history->next.find(word_id);
    if (next_history != token.history->next.end())
        token.history = next_history->second;
    else {
        token.history = new WordHistory(word_id, token.history);
        token.history->previous->next[word_id] = token.history;
        m_word_history_leafs.erase(token.history->previous);
        m_word_history_leafs.insert(token.history);
    }
}


bool
WordSubwordDecoder::update_lm_prob(Token &token, int word_id)
{
    static double ln_to_log10 = 1.0/log(10.0);

    double class_lm_prob = class_lm_score(token, word_id);
    if (class_lm_prob == MIN_LOG_PROB) return false;
    class_lm_prob += m_class_iw;

    double word_lm_prob = 0.0;
    token.lm_node = m_lm.score(token.lm_node, m_text_unit_id_to_ngram_symbol[word_id], word_lm_prob);
    word_lm_prob += m_word_iw;

    double subword_lm_prob = 0.0;
    vector<int> &subword_ids = m_word_id_to_subword_ngram_symbols[word_id];
    for (auto swit=subword_ids.begin(); swit != subword_ids.end(); ++swit)
        token.subword_lm_node = m_subword_lm.score(token.subword_lm_node, *swit, subword_lm_prob);
    subword_lm_prob += m_subword_iw;

    double interpolated_lp = add_log_domain_probs(word_lm_prob, class_lm_prob);
    interpolated_lp = add_log_domain_probs(interpolated_lp, subword_lm_prob);
    token.lm_log_prob += ln_to_log10 * interpolated_lp;

    return true;
}


void
WordSubwordDecoder::update_total_log_prob(Token &token)
{
    token.total_log_prob = token.am_log_prob + (m_lm_scale * token.lm_log_prob);
}


void
WordSubwordDecoder::apply_duration_model(Token &token, int node_idx)
{
    token.am_log_prob += m_duration_scale
                         * m_hmm_states[m_nodes[node_idx].hmm_state].duration.get_log_prob(token.dur);
}


void
WordSubwordDecoder::update_lookahead_prob(Token &token, float new_lookahead_prob)
{
    token.lm_log_prob -= token.lookahead_log_prob;
    token.lm_log_prob += new_lookahead_prob;
    token.lookahead_log_prob = new_lookahead_prob;
}


double
WordSubwordDecoder::class_lm_score(Token &token, int word_id)
{
    if (m_class_membership_lookup[word_id].second == MIN_LOG_PROB) return MIN_LOG_PROB;

    double ll = m_class_membership_lookup[word_id].second;
    double ngram_score = 0.0;
    if (word_id == m_sentence_end_symbol_idx) {
        token.class_lm_node = m_class_lm.score(token.class_lm_node,
                                               m_class_lm.vocabulary_lookup["</s>"],
                                               ngram_score);
        token.class_lm_node = m_class_lm.sentence_start_node;
    }
    else
        token.class_lm_node = m_class_lm.score(token.class_lm_node,
                                               m_class_intmap[m_class_membership_lookup[word_id].first],
                                               ngram_score);

    return ll + ngram_score;
}


void
WordSubwordDecoder::add_sentence_ends(vector<Token> &tokens)
{
    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        Token &token = *tit;
        if (token.lm_node == m_lm.sentence_start_node) continue;
        m_active_histories.erase(token.history);
        update_lm_prob(token, m_sentence_end_symbol_idx);
        update_total_log_prob(token);
        advance_in_word_history(token, m_sentence_end_symbol_idx);
        m_active_histories.insert(token.history);
    }
}


void
WordSubwordDecoder::print_best_word_history(ostream &outf)
{
    print_word_history(get_best_token()->history, outf);
}


void
WordSubwordDecoder::print_word_history(WordHistory *history,
                            ostream &outf,
                            bool print_lm_probs)
{
    vector<int> subwords;
    while (true) {
        subwords.push_back(history->word_id);
        if (history->previous == nullptr) break;
        history = history->previous;
    }

    int lm_node = m_lm.root_node;
    float total_lp = 0.0;
    if (m_use_word_boundary_symbol) {
        for (auto swit = subwords.rbegin(); swit != subwords.rend(); ++swit) {
            outf << " " << m_text_units[*swit];
            if (print_lm_probs) {
                float lp = 0.0;
                lm_node = m_lm.score(lm_node, m_text_unit_id_to_ngram_symbol[*swit], lp);
                outf << "(" << lp << ")";
                total_lp += lp;
            }
        }
    }
    else {
        for (auto swit = subwords.rbegin(); swit != subwords.rend(); ++swit) {
            if (*swit >= 0)
                outf << " " << m_text_units[*swit];
        }
    }

    if (print_lm_probs) outf << endl << "total lm log: " << total_lp;
    outf << endl;
}