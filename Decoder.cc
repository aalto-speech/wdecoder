#include <algorithm>
#include <sstream>

#include "NowayHmmReader.hh"
#include "Decoder.hh"

using namespace std;


Decoder::Decoder()
{
    m_debug = 0;
    m_stats = 0;

    m_token_stats = 0;
    m_total_token_count = 0;

    m_duration_model_in_use = false;
    m_use_word_boundary_symbol = false;
    m_force_sentence_end = true;

    m_lm_scale = 0.0;
    m_duration_scale = 0.0;
    m_transition_scale = 0.0;
    m_token_count = 0;
    m_token_count_after_pruning = 0;
    m_word_boundary_symbol_idx = -1;
    m_sentence_begin_symbol_idx = -1;
    m_sentence_end_symbol_idx = -1;

    m_dropped_count = 0;
    m_global_beam_pruned_count = 0;
    m_word_end_beam_pruned_count = 0;
    m_node_beam_pruned_count = 0;
    m_max_state_duration_pruned_count = 0;
    m_histogram_pruned_count = 0;

    m_acoustics = nullptr;

    m_la = nullptr;

    m_best_log_prob = -1e20;
    m_best_word_end_prob = -1e20;
    m_histogram_bin_limit = 0;

    m_global_beam = 0.0;
    m_current_word_end_beam = 0.0;
    m_word_end_beam = 0.0;
    m_node_beam = 0.0;

    m_token_limit = 500000;
    m_active_node_limit = 50000;

    m_history_root = nullptr;
    m_state_history_root = nullptr;
    m_history_clean_frame_interval = 10;

    m_word_boundary_penalty = 0.0;
    m_max_state_duration = 80;

    m_ngram_state_sentence_begin = -1;
    m_decode_start_node = -1;
    m_frame_idx = -1;

    m_long_silence_loop_start_node = -1;
    m_long_silence_loop_end_node = -1;
}


Decoder::~Decoder()
{
}


void
Decoder::read_phone_model(string phnfname)
{
    ifstream phnf(phnfname);
    if (!phnf) throw string("Problem opening phone model.");

    int modelcount;
    NowayHmmReader::read(phnf, m_hmms, m_hmm_map, m_hmm_states, modelcount);
}


void
Decoder::read_duration_model(string durfname)
{
    ifstream durf(durfname);
    if (!durf) throw string("Problem opening duration model.");

    NowayHmmReader::read_durations(durf, m_hmms, m_hmm_states);
    m_duration_model_in_use = true;
}


void
Decoder::read_noway_lexicon(string lexfname)
{
    ifstream lexf(lexfname);
    if (!lexf) throw string("Problem opening noway lexicon file.");

    string line;
    int linei = 1;
    while (getline(lexf, line)) {
        string unit;
        vector<string> phones;

        string phone;
        stringstream ss(line);
        ss >> unit;
        while (ss >> phone) phones.push_back(phone);

        auto leftp = unit.find("(");
        if (leftp != string::npos) {
            auto rightp = unit.find(")");
            if (rightp == string::npos) throw string("Problem reading line " + linei);
            double prob = atof(unit.substr(leftp+1, rightp-leftp-1).c_str());
            unit = unit.substr(0, leftp);
        }

        for (auto pit = phones.begin(); pit != phones.end(); ++pit) {
            if (m_hmm_map.find(*pit) == m_hmm_map.end())
                throw "Unknown phone " + *pit;
        }

        if (m_subword_map.find(unit) == m_subword_map.end()) {
            m_subwords.push_back(unit);
            m_subword_map[unit] = m_subwords.size()-1;
            if (unit == "<s>") m_sentence_begin_symbol_idx = m_subwords.size()-1;
            if (unit == "</s>") m_sentence_end_symbol_idx = m_subwords.size()-1;
        }
        m_lexicon[unit] = phones;

        linei++;
    }
}


void
Decoder::read_lm(string lmfname)
{
    m_lm.read_arpa(lmfname);
    set_subword_id_ngram_symbol_mapping();
}


void
Decoder::read_dgraph(string fname)
{
    ifstream ginf(fname);
    if (!ginf) throw string("Problem opening decoder graph.");

    int node_idx, node_count, arc_count;
    string line;

    getline(ginf, line);
    stringstream ncl(line);
    ncl >> node_count;
    m_nodes.clear();
    m_nodes.resize(node_count);

    string ltype;
    for (unsigned int i=0; i<m_nodes.size(); i++) {
        getline(ginf, line);
        stringstream nss(line);
        nss >> ltype;
        if (ltype != "n") throw string("Problem reading graph.");
        Node &node = m_nodes[i];
        nss >> node_idx >> node.hmm_state >> node.word_id >> arc_count >> node.flags;
        if (node.flags & NODE_DECODE_START) m_decode_start_node = node_idx;
        node.arcs.resize(arc_count);
    }

    vector<int> node_arc_counts;
    node_arc_counts.resize(node_count, 0);
    while (getline(ginf, line)) {
        stringstream ass(line);
        int src_node, tgt_node;
        ass >> ltype >> src_node >> tgt_node;
        if (ltype != "a") throw string("Problem reading graph.");
        m_nodes[src_node].arcs[node_arc_counts[src_node]].target_node = tgt_node;
        node_arc_counts[src_node]++;
    }

    set_hmm_transition_probs();

    m_long_silence_loop_start_node = m_nodes.size()-1;
    for (auto ait = m_nodes.back().arcs.begin(); ait != m_nodes.back().arcs.end(); ++ait)
        if (ait->target_node != START_NODE)
            m_long_silence_loop_end_node = ait->target_node;
}


void
Decoder::set_hmm_transition_probs()
{
    for (unsigned int i=0; i<m_nodes.size(); i++) {

        Node &node = m_nodes[i];
        if (node.hmm_state == -1) continue;

        HmmState &state = m_hmm_states[node.hmm_state];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            if (ait->target_node == (int)i) ait->log_prob = state.transitions[0].log_prob;
            else ait->log_prob = state.transitions[1].log_prob;
        }
    }
}


void
Decoder::set_subword_id_ngram_symbol_mapping()
{
    m_subword_id_to_ngram_symbol.resize(m_subwords.size(), -1);
    for (unsigned int i=0; i<m_subwords.size(); i++) {
        string tmp(m_subwords[i]);
        m_subword_id_to_ngram_symbol[i] = m_lm.vocabulary_lookup[tmp];
    }
}


void
Decoder::recognize_lna_file(string lnafname,
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
            prune_state_history();
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
        map<int, Token> &node_tokens = m_recombined_tokens[*nit];
        for (auto tit = node_tokens.begin(); tit != node_tokens.end(); ++tit)
            tokens.push_back(tit->second);
    }

    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        Token &tok = *tit;
        if (m_duration_model_in_use && tok.dur > 1) apply_duration_model(tok, tok.node_idx);
        advance_in_state_history(tok, m_nodes[tok.node_idx].hmm_state);
        update_lookahead_prob(tok, 0.0);
        tok.total_log_prob = get_token_log_prob(tok);
    }
    if (m_force_sentence_end) add_sentence_ends(tokens);

    Token best_token = get_best_token(tokens);
    outf << lnafname << ":";
    print_word_history(best_token.history, outf, false);

    clear_word_history();
    clear_state_history();
    m_lna_reader.close();

    if (frame_count != nullptr) *frame_count = m_frame_idx;
    if (seconds != nullptr) *seconds = difftime(end_time, start_time);
    if (log_prob != nullptr) *log_prob = best_token.total_log_prob;
    if (am_prob != nullptr) *am_prob = best_token.am_log_prob;
    if (lm_prob != nullptr) *lm_prob = best_token.lm_log_prob;
    if (total_token_count != nullptr) *total_token_count = m_total_token_count;
}


void
Decoder::initialize()
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
    m_active_state_histories.clear();
    Token tok;
    tok.lm_node = m_lm.sentence_start_node;
    tok.last_word_id = m_sentence_begin_symbol_idx;
    m_ngram_state_sentence_begin = tok.lm_node;
    tok.history = new WordHistory();
    tok.state_history = new StateHistory();
    tok.history->word_id = m_sentence_begin_symbol_idx;
    m_history_root = tok.history;
    tok.node_idx = m_decode_start_node;
    m_active_nodes.insert(m_decode_start_node);
    m_word_history_leafs.insert(tok.history);
    if (m_use_word_boundary_symbol) {
        advance_in_word_history(tok, m_word_boundary_symbol_idx);
        float dummy;
        tok.lm_node = m_lm.score(tok.lm_node, m_subword_id_to_ngram_symbol[m_word_boundary_symbol_idx], dummy);
        m_ngram_state_sentence_begin = tok.lm_node;
        tok.last_word_id = m_word_boundary_symbol_idx;
    }
    m_active_histories.insert(tok.history);
    m_active_state_histories.insert(tok.state_history);
    m_recombined_tokens[m_decode_start_node][tok.lm_node] = tok;

    m_total_token_count = 0;
}


void
Decoder::reset_frame_variables()
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
    m_active_state_histories.clear();
    fill(m_best_node_scores.begin(), m_best_node_scores.end(), -1e20);
}


bool descending_node_sort(const pair<int, float> &i, const pair<int, float> &j)
{
    return (i.second > j.second);
}


void
Decoder::active_nodes_sorted_by_best_lp(vector<int> &nodes)
{
    nodes.clear();
    vector<pair<int, float> > sorted_nodes;
    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit)
        sorted_nodes.push_back(make_pair(*nit, m_best_node_scores[*nit]));
    sort(sorted_nodes.begin(), sorted_nodes.end(), descending_node_sort);
    for (auto snit = sorted_nodes.begin(); snit != sorted_nodes.end(); ++snit)
        nodes.push_back(snit->first);
}


void
Decoder::propagate_tokens()
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
Decoder::prune_tokens(bool collect_active_histories)
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
        map<int, Token> &node_tokens = m_recombined_tokens[tit->node_idx];
        auto bntit = node_tokens.find(tit->lm_node);
        if (bntit != node_tokens.end()) {
            if (tit->total_log_prob > bntit->second.total_log_prob) {
                histogram[bntit->second.histogram_bin]--;
                histogram[tit->histogram_bin]++;
                bntit->second = *tit;
            }
            m_dropped_count++;
        }
        else {
            node_tokens[tit->lm_node] = *tit;
            m_active_nodes.insert(tit->node_idx);
            histogram[tit->histogram_bin]++;
            m_token_count_after_pruning++;
        }

        if (collect_active_histories) {
            m_active_histories.insert(tit->history);
            m_active_state_histories.insert(tit->state_history);
        }
    }

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
Decoder::move_token_to_node(Token token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead)
{
    token.am_log_prob += m_transition_scale * transition_score;

    Node &node = m_nodes[node_idx];

    if (token.node_idx == node_idx) {
        token.dur++;
        if (token.dur > m_max_state_duration && node.hmm_state > 3) {
            m_max_state_duration_pruned_count++;
            return;
        }
    }
    else {
        // Apply duration model for previous state if moved out from a hmm state
        if (m_duration_model_in_use && m_nodes[token.node_idx].hmm_state != -1)
            apply_duration_model(token, token.node_idx);
        advance_in_state_history(token, m_nodes[token.node_idx].hmm_state);
        token.node_idx = node_idx;
        token.dur = 1;
    }

    // HMM node
    if (node.hmm_state != -1) {

        if (update_lookahead)
            update_lookahead_prob(token, m_la->get_lookahead_score(node_idx, token.last_word_id));

        token.am_log_prob += m_acoustics->log_prob(node.hmm_state);

        token.total_log_prob = get_token_log_prob(token);
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
        token.lm_node = m_lm.score(token.lm_node, m_subword_id_to_ngram_symbol[node.word_id], token.lm_log_prob);
        token.last_word_id = node.word_id;

        if (update_lookahead)
            update_lookahead_prob(token, m_la->get_lookahead_score(node_idx, token.last_word_id));

        token.total_log_prob = get_token_log_prob(token);
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }

        advance_in_word_history(token, node.word_id);
        token.word_end = true;

        if (node.word_id == m_sentence_end_symbol_idx) {
            token.lm_node = m_ngram_state_sentence_begin;
            if (m_use_word_boundary_symbol) {
                advance_in_word_history(token, m_word_boundary_symbol_idx);
                token.last_word_id = m_word_boundary_symbol_idx;
            }

            if (update_lookahead)
                update_lookahead_prob(token, m_la->get_lookahead_score(node_idx, token.last_word_id));
            token.total_log_prob = get_token_log_prob(token);
        }
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
        move_token_to_node(token, ait->target_node, ait->log_prob, ait->update_lookahead);
}


Decoder::Token*
Decoder::get_best_token()
{
    Token *best_token = nullptr;

    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit) {
        map<int, Token> & node_tokens = m_recombined_tokens[*nit];
        for (auto tit = node_tokens.begin(); tit != node_tokens.end(); ++tit) {
            if (best_token == nullptr)
                best_token = &(tit->second);
            else if (tit->second.total_log_prob > best_token->total_log_prob)
                best_token = &(tit->second);
        }
    }

    return best_token;
}


Decoder::Token
Decoder::get_best_token(vector<Token> &tokens)
{
    Token *best_token = nullptr;

    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        if (best_token == nullptr)
            best_token = &(*tit);
        else if (tit->total_log_prob > best_token->total_log_prob)
            best_token = &(*tit);
    }

    return *best_token;
}


float
Decoder::get_token_log_prob(const Token &token)
{
    return (token.am_log_prob + m_lm_scale * token.lm_log_prob);
}


void
Decoder::advance_in_word_history(Token &token, int word_id)
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


void
Decoder::advance_in_state_history(Token &token, int hmm_state)
{
    auto next_history = token.state_history->next.find(hmm_state);
    if (next_history != token.state_history->next.end())
        token.state_history = next_history->second;
    else {
        token.state_history = new StateHistory(hmm_state, token.state_history);
        token.state_history->previous->next[hmm_state] = token.state_history;
        m_state_history_leafs.erase(token.state_history->previous);
        m_state_history_leafs.insert(token.state_history);
    }

    if (token.am_log_prob > token.state_history->best_am_log_prob) {
        token.state_history->best_am_log_prob = token.am_log_prob;
        token.state_history->end_frame = m_frame_idx-1;
        token.state_history->start_frame = m_frame_idx-token.dur-1;
    }
}


void
Decoder::apply_duration_model(Token &token, int node_idx)
{
    token.am_log_prob += m_duration_scale
                         * m_hmm_states[m_nodes[node_idx].hmm_state].duration.get_log_prob(token.dur);
}


void
Decoder::update_lookahead_prob(Token &token, float new_lookahead_prob)
{
    token.lm_log_prob -= token.lookahead_log_prob;
    token.lm_log_prob += new_lookahead_prob;
    token.lookahead_log_prob = new_lookahead_prob;
}


void
Decoder::add_sentence_ends(vector<Token> &tokens)
{
    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        Token &token = *tit;
        if (token.lm_node == m_lm.sentence_start_node) continue;
        m_active_histories.erase(token.history);
        if (m_use_word_boundary_symbol && token.history->word_id != m_subword_map[m_word_boundary_symbol]) {
            token.lm_node = m_lm.score(token.lm_node,
                                       m_subword_id_to_ngram_symbol[m_subword_map[m_word_boundary_symbol]], token.lm_log_prob);
            token.total_log_prob = get_token_log_prob(token);
            advance_in_word_history(token, m_subword_map[m_word_boundary_symbol]);
        }
        token.lm_node = m_lm.score(token.lm_node, m_subword_id_to_ngram_symbol[m_sentence_end_symbol_idx], token.lm_log_prob);
        token.total_log_prob = get_token_log_prob(token);
        advance_in_word_history(token, m_sentence_end_symbol_idx);
        m_active_histories.insert(token.history);
    }
}


void
Decoder::prune_word_history()
{
    for (auto whlnit = m_word_history_leafs.begin(); whlnit != m_word_history_leafs.end(); ) {
        WordHistory *wh = *whlnit;
        if (m_active_histories.find(wh) == m_active_histories.end()) {
            m_word_history_leafs.erase(whlnit++);
            while (true) {
                WordHistory *tmp = wh;
                wh = wh->previous;
                wh->next.erase(tmp->word_id);
                delete tmp;
                if (wh == nullptr || wh->next.size() > 0) break;
                if (m_active_histories.find(wh) != m_active_histories.end()) {
                    m_word_history_leafs.insert(wh);
                    break;
                }
            }
        }
        else ++whlnit;
    }
}


void
Decoder::clear_word_history()
{
    for (auto whlnit = m_word_history_leafs.begin(); whlnit != m_word_history_leafs.end(); ++whlnit) {
        WordHistory *wh = *whlnit;
        while (wh != nullptr) {
            if (wh->next.size() > 0) break;
            WordHistory *tmp = wh;
            wh = wh->previous;
            if (wh != nullptr) wh->next.erase(tmp->word_id);
            delete tmp;
        }
    }
    m_word_history_leafs.clear();
    m_active_histories.clear();
}


void
Decoder::prune_state_history()
{
    for (auto shlnit = m_state_history_leafs.begin(); shlnit != m_state_history_leafs.end(); ) {
        StateHistory *sh = *shlnit;
        if (m_active_state_histories.find(sh) == m_active_state_histories.end()) {
            m_state_history_leafs.erase(shlnit++);
            while (true) {
                StateHistory *tmp = sh;
                sh = sh->previous;
                sh->next.erase(tmp->hmm_state);
                delete tmp;
                if (sh == nullptr || sh->next.size() > 0) break;
                if (m_active_state_histories.find(sh) != m_active_state_histories.end()) {
                    m_state_history_leafs.insert(sh);
                    break;
                }
            }
        }
        else ++shlnit;
    }
}


void
Decoder::clear_state_history()
{
    for (auto shlnit = m_state_history_leafs.begin(); shlnit != m_state_history_leafs.end(); ++shlnit) {
        StateHistory *sh = *shlnit;
        while (sh != nullptr) {
            if (sh->next.size() > 0) break;
            StateHistory *tmp = sh;
            sh = sh->previous;
            if (sh != nullptr) sh->next.erase(tmp->hmm_state);
            delete tmp;
        }
    }
    m_state_history_leafs.clear();
    m_active_state_histories.clear();
}


void
Decoder::print_certain_word_history(ostream &outf)
{
    WordHistory *hist = m_history_root;
    while (true) {
        if (hist->word_id >= 0)
            outf << m_subwords[hist->word_id] << " ";
        if (hist->next.size() > 1 || hist->next.size() == 0) break;
        else hist = hist->next.begin()->second;
    }
    outf << endl;
}


void
Decoder::print_best_word_history(ostream &outf)
{
    print_word_history(get_best_token()->history, outf);
}


void
Decoder::print_word_history(WordHistory *history,
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
            outf << " " << m_subwords[*swit];
            if (print_lm_probs) {
                float lp = 0.0;
                lm_node = m_lm.score(lm_node, m_subword_id_to_ngram_symbol[*swit], lp);
                outf << "(" << lp << ")";
                total_lp += lp;
            }
        }
    }
    else {
        for (auto swit = subwords.rbegin(); swit != subwords.rend(); ++swit) {
            if (*swit >= 0)
                outf << " " << m_subwords[*swit];
        }
    }

    if (print_lm_probs) outf << endl << "total lm log: " << total_lp;
    outf << endl;
}


void
Decoder::print_dot_digraph(vector<Node> &nodes,
                           ostream &fstr)
{
    fstr << "digraph {" << endl << endl;
    fstr << "\tnode [shape=ellipse,fontsize=30,fixedsize=false,width=0.95];" << endl;
    fstr << "\tedge [fontsize=12];" << endl;
    fstr << "\trankdir=LR;" << endl << endl;

    for (unsigned int nidx = 0; nidx < m_nodes.size(); ++nidx) {
        Node &nd = m_nodes[nidx];
        fstr << "\t" << nidx;
        if (nidx == START_NODE) fstr << " [label=\"start\"]" << endl;
        else if (nidx == END_NODE) fstr << " [label=\"end\"]" << endl;
        else if (nd.hmm_state != -1 && nd.word_id >= 0)
            fstr << " [label=\"" << nidx << ":" << nd.hmm_state << ", " << m_subwords[nd.word_id] << "\"]" << endl;
        else if (nd.hmm_state != -1 && nd.word_id == -1)
            fstr << " [label=\"" << nidx << ":"<< nd.hmm_state << "\"]" << endl;
        else if (nd.hmm_state == -1 && nd.word_id >= 0)
            fstr << " [label=\"" << nidx << ":"<< m_subwords[nd.word_id] << "\"]" << endl;
        else if (nd.hmm_state == -1 && nd.word_id == -2)
            fstr << " [label=\"" << nidx << ":dummy/wb\"]" << endl;
        else
            fstr << " [label=\"" << nidx << ":dummy\"]" << endl;
    }

    fstr << endl;
    for (unsigned int nidx = 0; nidx < m_nodes.size(); ++nidx) {
        Node &node = m_nodes[nidx];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            fstr << "\t" << nidx << " -> " << ait->target_node
                 << "[label=\"" << ait->log_prob << "\"];" << endl;
    }
    fstr << "}" << endl;
}


float
Decoder::score_state_path(string lnafname,
                          string sfname,
                          bool duration_model)
{
    cerr << "lna file: " << lnafname << endl;
    cerr << "state path file: " << sfname << endl;
    m_lna_reader.open_file(lnafname, 1024);
    m_acoustics = &m_lna_reader;

    ifstream sinf(sfname);
    if (!sinf) throw string("Problem opening state path file.");

    int start=-1, end=-1, state_idx=-1;
    string line;
    m_frame_idx = 0;

    float gmm_score = 0.0;
    float trans_score = 0.0;
    float dur_score = 0.0;
    float total_score = 0.0;
    while (getline(sinf, line)) {
        cerr << "frame: " << m_frame_idx << endl;
        stringstream ncl(line);
        ncl >> start >> end >> state_idx;
        int dur = end-start;

        if (state_idx != -1)
            trans_score += m_hmm_states[state_idx].transitions[1].log_prob;
        HmmState &state = m_hmm_states[state_idx];
        while (m_frame_idx < end) {
            m_lna_reader.go_to(m_frame_idx);
            gmm_score += m_acoustics->log_prob(state_idx);
            if (m_frame_idx != start)
                trans_score += state.transitions[0].log_prob;
            m_frame_idx++;
            total_score = gmm_score + trans_score + dur_score;
        }
        if (duration_model && m_duration_model_in_use)
            dur_score += m_duration_scale * m_hmm_states[state_idx].duration.get_log_prob(dur);

        total_score = gmm_score + trans_score + dur_score;

        cerr << m_frame_idx << "\t" << gmm_score << "\t" << trans_score << "\t" << total_score << endl;
    }

    total_score = gmm_score + trans_score + dur_score;
    cerr << "gmm: " << gmm_score << endl;
    cerr << "trans: " << trans_score << endl;
    cerr << "dur: " << dur_score << endl;
    cerr << "total: " << total_score << endl;

    m_lna_reader.close();
    return total_score;
}


void
Decoder::find_paths(vector<vector<int> > &paths,
                    vector<int> &words,
                    int curr_word_pos,
                    vector<int> curr_path,
                    int curr_node_idx)
{
    if (curr_node_idx == -1) {
        curr_node_idx = m_decode_start_node;
        curr_path.push_back(curr_node_idx);
        Node &node = m_nodes[curr_node_idx];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            if (ait->target_node == curr_node_idx) continue;
            find_paths(paths, words, curr_word_pos, curr_path, ait->target_node);
        }
        return;
    }

    curr_path.push_back(curr_node_idx);
    Node &node = m_nodes[curr_node_idx];

    if (node.word_id != -1) {
        if (node.word_id != words[curr_word_pos]) return;
        else curr_word_pos++;
        if (curr_word_pos == (int)words.size()) {
            paths.push_back(curr_path);
            return;
        }
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (ait->target_node == curr_node_idx) continue;
        find_paths(paths, words, curr_word_pos, curr_path, ait->target_node);
    }
}


void
Decoder::path_to_graph(vector<int> &path,
                       vector<Node> &nodes)
{
    for (int i=0; i<(int)path.size(); i++) {
        nodes.push_back(m_nodes[path[i]]);
        nodes.back().arcs.clear();
    }
    for (int i=0; i<(int)path.size()-1; i++) {
        if (nodes[i].hmm_state != -1) {
            nodes[i].arcs.resize(2);
            nodes[i].arcs[0].target_node = i;
            nodes[i].arcs[1].target_node = i+1;
        }
        else {
            nodes[i].arcs.resize(1);
            nodes[i].arcs[0].target_node = i+1;
        }
    }
}

