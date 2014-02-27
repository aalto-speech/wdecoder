#include <algorithm>
#include <cassert>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "NowayHmmReader.hh"
#include "Decoder.hh"
#include "io.hh"

using namespace std;
using namespace fsalm;

const int Decoder::WORD_BOUNDARY_IDENTIFIER = -2;


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
        }
        m_lexicon[unit] = phones;

        linei++;
    }
}


void
Decoder::read_lm(string lmfname)
{
    m_lm.read_arpa(io::Stream(lmfname, "r").file, true);
    m_lm.trim();
    set_subword_id_fsa_symbol_mapping();
}


void
Decoder::read_la_lm(string lmfname)
{
    m_la_lm.read_arpa(io::Stream(lmfname, "r").file, true);
    m_la_lm.trim();
    set_subword_id_la_fsa_symbol_mapping();
    set_unigram_la_scores();
    m_unigram_la_in_use = true;
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
    for (int i=0; i<m_nodes.size(); i++) {
        getline(ginf, line);
        stringstream nss(line);
        nss >> ltype;
        if (ltype != "n") throw string("Problem reading graph.");
        Node &node = m_nodes[i];
        nss >> node_idx >> node.hmm_state >> node.word_id >> arc_count >> node.flags;
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

    add_silence_hmms(m_nodes);
    set_hmm_transition_probs(m_nodes);
    set_word_boundaries();
}


void
Decoder::read_config(string cfgfname)
{
    ifstream cfgf(cfgfname);
    if (!cfgf) throw string("Problem opening configuration file: ") + cfgfname;

    string line;
    while (getline(cfgf, line)) {
        if (!line.length()) continue;
        stringstream ss(line);
        string parameter, val;
        ss >> parameter;
        if (parameter == "lm_scale") ss >> m_lm_scale;
        else if (parameter == "token_limit") ss >> m_token_limit;
        else if (parameter == "history_limit") ss >> m_history_limit;
        else if (parameter == "node_limit") ss >> m_active_node_limit;
        else if (parameter == "duration_scale") ss >> m_duration_scale;
        else if (parameter == "transition_scale") ss >> m_transition_scale;
        else if (parameter == "global_beam") ss >> m_global_beam;
        else if (parameter == "acoustic_beam") ss >> m_acoustic_beam;
        else if (parameter == "history_beam") ss >> m_history_beam;
        else if (parameter == "word_end_beam") ss >> m_word_end_beam;
        else if (parameter == "word_boundary_penalty") ss >> m_word_boundary_penalty;
        else if (parameter == "history_clean_frame_interval") ss >> m_history_clean_frame_interval;
        else if (parameter == "force_sentence_end") {
            string force_str;
            ss >> force_str;
            m_force_sentence_end = true ? force_str == "true": false;
        }
        else if (parameter == "word_boundary_symbol") {
            m_use_word_boundary_symbol = true;
            ss >> m_word_boundary_symbol;
            m_word_boundary_symbol_idx = m_subword_map[m_word_boundary_symbol];
        }
        else if (parameter == "debug") ss >> m_debug;
        else if (parameter == "stats") ss >> m_stats;
        else throw string("Unknown parameter: ") + parameter;
    }

    cfgf.close();
}


void
Decoder::print_config(ostream &outf)
{
    outf << std::boolalpha;
    outf << "lm scale: " << m_lm_scale << endl;
    //outf << "history limit: " << m_history_limit << endl;
    outf << "active node limit: " << m_active_node_limit << endl;
    outf << "token limit: " << m_token_limit << endl;
    outf << "duration scale: " << m_duration_scale << endl;
    outf << "transition scale: " << m_transition_scale << endl;
    outf << "force sentence end: " << m_force_sentence_end << endl;
    outf << "use word boundary symbol: " << m_use_word_boundary_symbol << endl;
    if (m_use_word_boundary_symbol)
        outf << "word boundary symbol: " << m_word_boundary_symbol << endl;
    outf << "global beam: " << m_global_beam << endl;
    outf << "acoustic beam: " << m_acoustic_beam << endl;
    //outf << "acoustic history beam: " << m_history_beam << endl;
    outf << "word end beam: " << m_word_end_beam << endl;
    outf << "word boundary penalty: " << m_word_boundary_penalty << endl;
    outf << "history clean frame interval: " << m_history_clean_frame_interval << endl;
}


void
Decoder::add_silence_hmms(std::vector<Node> &nodes,
                          bool long_silence,
                          bool short_silence)
{
    Node &end_node = nodes[END_NODE];
    end_node.arcs.clear();

    if (long_silence) {
        string long_silence("__");
        int hmm_index = m_hmm_map[long_silence];
        Hmm &hmm = m_hmms[hmm_index];

        nodes.resize(nodes.size()+1);
        nodes.back().hmm_state = -1;
        nodes.back().word_id = m_subword_map[string("</s>")];
        DECODE_START_NODE = nodes.size()-1;
        nodes[END_NODE].arcs.resize(nodes[END_NODE].arcs.size()+1);
        nodes[END_NODE].arcs.back().target_node = DECODE_START_NODE;

        int node_idx = END_NODE;
        for (int sidx = 2; sidx < hmm.states.size(); ++sidx) {
            nodes.resize(nodes.size()+1);
            nodes.back().hmm_state = hmm.states[sidx].model;
            nodes.back().arcs.resize(nodes.back().arcs.size()+1);
            nodes.back().arcs.back().target_node = nodes.size()-1;
            nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
            nodes[node_idx].arcs.back().target_node = nodes.size()-1;
            if (sidx == 2) {
                nodes[DECODE_START_NODE].arcs.resize(1);
                nodes[DECODE_START_NODE].arcs.back().target_node = nodes.size()-1;
            }
            node_idx = nodes.size()-1;
        }

        nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
        nodes[node_idx].arcs.back().target_node = START_NODE;
    }

    if (short_silence) {
        string short_silence("_");
        int hmm_index = m_hmm_map[short_silence];
        Hmm &hmm = m_hmms[hmm_index];

        int node_idx = END_NODE;
        for (int sidx = 2; sidx < hmm.states.size(); ++sidx) {
            nodes.resize(nodes.size()+1);
            nodes.back().hmm_state = hmm.states[sidx].model;
            nodes.back().arcs.resize(nodes.back().arcs.size()+1);
            nodes.back().arcs.back().target_node = nodes.size()-1;
            nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
            nodes[node_idx].arcs.back().target_node = nodes.size()-1;
            node_idx = nodes.size()-1;
        }
        nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
        nodes[node_idx].arcs.back().target_node = START_NODE;
    }
}


void
Decoder::set_hmm_transition_probs(std::vector<Node> &nodes)
{
    for (int i=0; i<nodes.size(); i++) {

        Node &node = nodes[i];
        if (node.hmm_state == -1) continue;

        HmmState &state = m_hmm_states[node.hmm_state];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            if (ait->target_node == i) ait->log_prob = state.transitions[0].log_prob;
            else ait->log_prob = state.transitions[1].log_prob;
        }
    }
}


void
Decoder::set_subword_id_fsa_symbol_mapping()
{
    m_subword_id_to_fsa_symbol.resize(m_subwords.size(), -1);
    for (int i=0; i<m_subwords.size(); i++) {
        string tmp(m_subwords[i]);
        m_subword_id_to_fsa_symbol[i] = m_lm.symbol_map().index(tmp);
        if (tmp == "</s>") SENTENCE_END_WORD_ID = i;
    }
}


void
Decoder::set_subword_id_la_fsa_symbol_mapping()
{
    m_subword_id_to_la_fsa_symbol.resize(m_subwords.size(), -1);
    for (int i=0; i<m_subwords.size(); i++) {
        string tmp(m_subwords[i]);
        m_subword_id_to_la_fsa_symbol[i] = m_la_lm.symbol_map().index(tmp);
    }
}


void
Decoder::recognize_lna_file(string lnafname,
                            ostream &outf,
                            int *frame_count,
                            double *seconds,
                            double *log_prob,
                            double *am_prob,
                            double *lm_prob)
{
    m_lna_reader.open_file(lnafname, 1024);
    m_acoustics = &m_lna_reader;
    initialize();

    time_t start_time, end_time;
    time(&start_time);
    int frame_idx = 0;
    while (m_lna_reader.go_to(frame_idx)) {

        reset_frame_variables();
        propagate_tokens();
        prune_tokens();
        //if (frame_idx % m_history_clean_frame_interval == 0) prune_word_history();

        if (m_stats) {
            cerr << endl << "recognized frame: " << frame_idx << endl;
            cerr << "current global beam: " << m_global_beam << endl;
            cerr << "current acoustic beam: " << m_acoustic_beam << endl;
            cerr << "tokens pruned by global beam: " << m_global_beam_pruned_count << endl;
            cerr << "tokens pruned by acoustic beam: " << m_acoustic_beam_pruned_count << endl;
            cerr << "tokens dropped by max assumption: " << m_dropped_count << endl;
            //cerr << "tokens pruned by acoustic history beam: " << m_history_beam_pruned_count << endl;
            cerr << "tokens pruned by word end beam: " << m_word_end_beam_pruned_count << endl;
            cerr << "best log probability: " << m_best_log_prob << endl;
            cerr << "number of active nodes: " << m_active_nodes.size() << endl;
            print_best_word_history(outf);
        }

        frame_idx++;
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
        update_lookahead_prob(tok, 0.0);
        tok.total_log_prob = get_token_log_prob(tok.am_log_prob, tok.lm_log_prob);
    }
    if (m_force_sentence_end) add_sentence_ends(tokens);

    Token best_token = get_best_token(tokens);
    outf << lnafname << ":";
    print_word_history(best_token.history, outf, false);

    clear_word_history();
    m_lna_reader.close();

    if (frame_count != nullptr) *frame_count = frame_idx;
    if (seconds != nullptr) *seconds = difftime(end_time, start_time);
    if (log_prob != nullptr) *log_prob = best_token.total_log_prob;
    if (am_prob != nullptr) *am_prob = best_token.am_log_prob;
    if (lm_prob != nullptr) *lm_prob = best_token.lm_log_prob;
}


void
Decoder::initialize()
{
    m_word_history_leafs.clear();
    m_raw_tokens.clear();
    m_raw_tokens.reserve(1000000);
    m_recombined_tokens.clear();
    m_recombined_tokens.resize(m_nodes.size());
    m_best_node_scores.resize(m_nodes.size(), -1e20);
    m_active_nodes.clear();
    m_active_histories.clear();
    Token tok;
    tok.fsa_lm_node = m_lm.initial_node_id();
    tok.history = new WordHistory();
    tok.node_idx = DECODE_START_NODE;
    m_active_nodes.insert(DECODE_START_NODE);
    m_recombined_tokens[DECODE_START_NODE][m_lm.initial_node_id()] = tok;
    m_active_histories.insert(tok.history);
    m_word_history_leafs.insert(tok.history);
}


void
Decoder::reset_frame_variables()
{
    m_token_count = 0;
    m_propagated_count = 0;
    m_best_log_prob = -1e20;
    m_best_am_log_prob = -1e20;
    m_best_word_end_prob = -1e20;
    m_global_beam_pruned_count = 0;
    m_acoustic_beam_pruned_count = 0;
    m_max_state_duration_pruned_count = 0;
    m_history_beam_pruned_count = 0;
    m_word_end_beam_pruned_count = 0;
    m_state_beam_pruned_count = 0;
    m_dropped_count = 0;
    m_token_count_after_pruning = 0;
}


bool descending_node_sort(const std::pair<int, float> &i, const std::pair<int, float> &j)
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
Decoder::propagate_tokens(void)
{
    vector<int> sorted_active_nodes;
    active_nodes_sorted_by_best_lp(sorted_active_nodes);

    int node_count = 0;
    for (auto nit = sorted_active_nodes.begin(); nit != sorted_active_nodes.end(); ++nit) {
        Node &node = m_nodes[*nit];
        for (auto tit = m_recombined_tokens[*nit].begin(); tit != m_recombined_tokens[*nit].end(); ++tit) {
            m_token_count++;
            tit->second.word_end = false;
            for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
                move_token_to_node(tit->second, ait->target_node, ait->log_prob);
                m_propagated_count++;
            }
        }
        node_count++;
        if (m_token_count > m_token_limit) break;
        if (node_count > m_active_node_limit) break;
    }

    for (auto nit = sorted_active_nodes.begin(); nit != sorted_active_nodes.end(); ++nit)
        m_recombined_tokens[*nit].clear();

    if (m_stats) {
        cerr << "token count before propagation: " << m_token_count << endl;
        cerr << "propagated token count: " << m_propagated_count << endl;
    }
}


void
Decoder::prune_tokens(void)
{
    vector<Token> pruned_tokens;
    pruned_tokens.reserve(50000);

    // Global beam pruning
    // Global acoustic beam pruning
    // History acoustic beam pruning
    // Word end beam pruning
    float current_glob_beam = m_best_log_prob - m_global_beam;
    float current_acoustic_beam = m_best_am_log_prob - m_acoustic_beam;
    float current_word_end_beam = m_best_word_end_prob - m_word_end_beam;
    for (int i=0; i<m_raw_tokens.size(); i++) {
        Token &tok = m_raw_tokens[i];
        if (tok.total_log_prob < current_glob_beam)
            m_global_beam_pruned_count++;
        if (tok.am_log_prob < current_acoustic_beam)
            m_acoustic_beam_pruned_count++;
        else if (tok.word_end && tok.total_log_prob < current_word_end_beam)
            m_word_end_beam_pruned_count++;
        else
            pruned_tokens.push_back(tok);
    }
    m_raw_tokens.clear();

    // Collect best tokens for each node/fsa state pair
    m_active_nodes.clear();
    m_active_histories.clear();
    fill(m_best_node_scores.begin(), m_best_node_scores.end(), -1e20);
    for (auto tit = pruned_tokens.begin(); tit != pruned_tokens.end(); tit++) {
        std::map<int, Token> &node_tokens = m_recombined_tokens[tit->node_idx];
        auto bntit = node_tokens.find(tit->fsa_lm_node);
        if (bntit != node_tokens.end()) {
            if (tit->total_log_prob > bntit->second.total_log_prob) {
                bntit->second = *tit;
                //m_active_histories.insert(tit->history);
                m_best_node_scores[tit->node_idx] = max(m_best_node_scores[tit->node_idx], tit->total_log_prob);
            }
            m_dropped_count++;
        }
        else {
            node_tokens[tit->fsa_lm_node] = *tit;
            m_active_nodes.insert(tit->node_idx);
            m_token_count_after_pruning++;
            m_best_node_scores[tit->node_idx] = max(m_best_node_scores[tit->node_idx], tit->total_log_prob);
            //m_active_histories.insert(tit->history);
        }
    }

    if (m_stats) cerr << "token count after pruning: " << m_token_count_after_pruning << endl;
}


void
Decoder::move_token_to_node(Token token,
                            int node_idx,
                            float transition_score)
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
        token.node_idx = node_idx;
        token.dur = 1;
    }

    if (m_unigram_la_in_use) update_lookahead_prob(token, node.unigram_la_score);
    //if ((node.flags & NODE_FAN_OUT_DUMMY) || node_idx == END_NODE) token.word_end = true;

    // HMM node
    if (node.hmm_state != -1) {

        token.am_log_prob += m_acoustics->log_prob(node.hmm_state);
        if (token.am_log_prob < (m_best_am_log_prob-m_acoustic_beam)) {
             m_acoustic_beam_pruned_count++;
             return;
        }

        token.total_log_prob = get_token_log_prob(token.am_log_prob, token.lm_log_prob);
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }

        m_best_log_prob = max(m_best_log_prob, token.total_log_prob);
        m_best_am_log_prob = max(m_best_am_log_prob, token.am_log_prob);
        if (token.word_end) m_best_word_end_prob = max(m_best_word_end_prob, token.total_log_prob);
        m_raw_tokens.push_back(token);
        return;
    }

    // LM node, update LM score
    // Update history
    if (node.word_id != -1) {
        token.fsa_lm_node = m_lm.walk(token.fsa_lm_node, m_subword_id_to_fsa_symbol[node.word_id], &token.lm_log_prob);
        if (node.word_id == SENTENCE_END_WORD_ID) token.fsa_lm_node = m_lm.initial_node_id();
        if (node.word_id != m_word_boundary_symbol_idx) token.word_end = true;
        token.total_log_prob = get_token_log_prob(token.am_log_prob, token.lm_log_prob);
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }
        advance_in_history(token, node.word_id);
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
        move_token_to_node(token, ait->target_node, ait->log_prob);
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
Decoder::get_token_log_prob(float am_score, float lm_score)
{
    return (am_score + m_lm_scale * lm_score);
}


void
Decoder::advance_in_history(Token &token, int word_id)
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
        if (token.fsa_lm_node == m_lm.initial_node_id()) continue;
        m_active_histories.erase(token.history);
        if (m_use_word_boundary_symbol && token.history->word_id != m_subword_map[m_word_boundary_symbol]) {
            token.fsa_lm_node = m_lm.walk(token.fsa_lm_node,
                    m_subword_id_to_fsa_symbol[m_subword_map[m_word_boundary_symbol]], &token.lm_log_prob);
            token.total_log_prob = get_token_log_prob(token.am_log_prob, token.lm_log_prob);
            advance_in_history(token, m_subword_map[m_word_boundary_symbol]);
        }
        token.fsa_lm_node = m_lm.walk(token.fsa_lm_node, m_subword_id_to_fsa_symbol[SENTENCE_END_WORD_ID], &token.lm_log_prob);
        token.total_log_prob = get_token_log_prob(token.am_log_prob, token.lm_log_prob);
        advance_in_history(token, SENTENCE_END_WORD_ID);
        m_active_histories.insert(token.history);
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

    int fsa_lm_node = m_lm.initial_node_id();
    float total_lp = 0.0;

    if (m_use_word_boundary_symbol) {
        outf << " <s>";
        for (auto swit = subwords.rbegin(); swit != subwords.rend(); ++swit) {
            if ((*swit) >= 0) outf << " " << m_subwords[*swit];
            if (print_lm_probs) {
                float lp = 0.0;
                fsa_lm_node = m_lm.walk(fsa_lm_node, m_subword_id_to_fsa_symbol[*swit], &lp);
                outf << "(" << lp << ")";
                total_lp += lp;
            }
        }
    }
    else {
        for (auto swit = subwords.rbegin(); swit != subwords.rend(); ++swit) {
            if (*swit >= 0) outf << m_subwords[*swit];
            else if (*swit == -2) outf << " ";
        }
    }

    if (print_lm_probs) outf << endl << "total lm log: " << total_lp;
    outf << endl;
}


void
Decoder::print_dot_digraph(vector<Node> &nodes, ostream &fstr)
{
    fstr << "digraph {" << endl << endl;
    fstr << "\tnode [shape=ellipse,fontsize=30,fixedsize=false,width=0.95];" << endl;
    fstr << "\tedge [fontsize=12];" << endl;
    fstr << "\trankdir=LR;" << endl << endl;

    for (int nidx = 0; nidx < m_nodes.size(); ++nidx) {
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
    for (int nidx = 0; nidx < m_nodes.size(); ++nidx) {
        Node &node = m_nodes[nidx];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            fstr << "\t" << nidx << " -> " << ait->target_node
                 << "[label=\"" << ait->log_prob << "\"];" << endl;
    }
    fstr << "}" << endl;
}


void
Decoder::set_word_boundaries()
{
    if (m_use_word_boundary_symbol) {
        int wbcount = 0;
        for (auto nit = m_nodes.begin(); nit != m_nodes.end(); ++nit) {
            if (nit->flags & NODE_FAN_OUT_DUMMY) {
                wbcount++;
                nit->word_id = m_subword_map[m_word_boundary_symbol];
            }
        }
        cerr << "word boundary count: " << wbcount+1 << endl;
        m_nodes[START_NODE].word_id = m_subword_map[m_word_boundary_symbol];
    }
    else {
        int wbcount = 0;
        for (auto nit = m_nodes.begin(); nit != m_nodes.end(); ++nit) {
            if (nit->flags & NODE_FAN_IN_DUMMY) {
                wbcount++;
                nit->word_id = WORD_BOUNDARY_IDENTIFIER;
            }
        }
        cerr << "word boundary count: " << wbcount+1 << endl;
        m_nodes[START_NODE].word_id = WORD_BOUNDARY_IDENTIFIER;
    }
}


void
Decoder::find_successor_words(int node_idx, map<int, set<int> > &word_ids, bool start_node)
{
    Node &node = m_nodes[node_idx];

    if (!start_node && node.word_id != -1) {
        auto wids = word_ids.find(node.word_id);
        if (wids == word_ids.end()) {
            set<int> tmp;
            tmp.insert(node_idx);
            word_ids[node.word_id] = tmp;
        }
        else {
            word_ids[node.word_id].insert(node_idx);
        }
        return;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        int target_node = ait->target_node;
        if (target_node == node_idx) continue;
        find_successor_words(target_node, word_ids, false);
    }
}


void
Decoder::find_successor_words(vector<map<int, set<int> > > &nodes)
{
    nodes.resize(m_nodes.size());
    for (int i=0; i<m_nodes.size(); i++) {
        //if (i % 10000 == 0) cerr << "node: " << i << endl;
        map<int, set<int> > word_ids;
        find_successor_words(i, word_ids, true);
        nodes[i] = word_ids;
    }
}


void
Decoder::set_unigram_la_scores()
{
    unique_ptr<vector<map<int, set<int> > > > successors(new vector<map<int, set<int> > >);
    find_successor_words(*successors);

    for (int i=0; i<m_nodes.size(); i++) {

        map<int, set<int> > &word_ids = (*successors)[i];
        //cerr << "node: " << i << "\t" << word_ids.size() << endl;

        if (word_ids.size() > 1) {
            float node_best_la_prob = -1e20;
            for (auto wit = word_ids.begin(); wit != word_ids.end(); ++wit) {
                float la_lm_prob = 0.0;
                m_la_lm.walk(m_la_lm.empty_node_id(), m_subword_id_to_la_fsa_symbol[wit->first], &la_lm_prob);
                node_best_la_prob = max(node_best_la_prob, la_lm_prob);
            }
            m_nodes[i].unigram_la_score = node_best_la_prob;
        }
        else if (word_ids.size() == 1) {
            // First step with unigram la
            float la_lm_prob = 0.0;
            m_la_lm.walk(m_la_lm.empty_node_id(), m_subword_id_to_la_fsa_symbol[word_ids.begin()->first], &la_lm_prob);
            float dummy = 0.0;
            int fsa_state = m_lm.walk(m_lm.empty_node_id(), m_subword_id_to_fsa_symbol[word_ids.begin()->first], &dummy);

            // Possible following word ids without branching with normal lm
            unique_ptr<set<int> > next_word_ids(new set<int>);
            unique_ptr<set<int> > next_nodes(new set<int>);
            for (auto nit = word_ids.begin()->second.begin(); nit != word_ids.begin()->second.end(); ++nit) {
                map<int, set<int> > &temp_word_ids = (*successors)[*nit];
                for (auto wnit = temp_word_ids.begin(); wnit != temp_word_ids.end(); ++wnit) {
                    next_word_ids->insert(wnit->first);
                    next_nodes->insert(wnit->second.begin(), wnit->second.end());
                }
            }

            while (next_word_ids->size() == 1) {
                fsa_state = m_lm.walk(fsa_state, m_subword_id_to_fsa_symbol[*(next_word_ids->begin())], &la_lm_prob);
                unique_ptr<set<int> > temp_nodes(new set<int>(*next_nodes));
                next_word_ids->clear();
                next_nodes->clear();
                for (auto tnit = temp_nodes->begin(); tnit != temp_nodes->end(); ++tnit) {
                    map<int, set<int> > &temp_word_ids = (*successors)[*tnit];
                    for (auto wnit = temp_word_ids.begin(); wnit != temp_word_ids.end(); ++wnit) {
                        next_word_ids->insert(wnit->first);
                        next_nodes->insert(wnit->second.begin(), wnit->second.end());
                    }
                }
            }

            // First branching with respect to word ids, normal lm
            float node_best_la_prob = -1e20;
            for (auto wit = next_word_ids->begin(); wit != next_word_ids->end(); ++wit) {
                float next_la_lm_prob = 0.0;
                m_lm.walk(fsa_state, m_subword_id_to_fsa_symbol[*wit], &next_la_lm_prob);
                node_best_la_prob = max(node_best_la_prob, next_la_lm_prob);
            }

            m_nodes[i].unigram_la_score = node_best_la_prob + la_lm_prob;
        }
    }
}


float
Decoder::score_state_path(string lnafname,
                          string sfname,
                          bool duration_model)
{
    m_lna_reader.open_file(lnafname, 1024);
    m_acoustics = &m_lna_reader;

    ifstream sinf(sfname);
    if (!sinf) throw string("Problem opening state path file.");

    int start, end, state_idx;
    string line;
    int frame_idx = 0;

    float gmm_score = 0.0;
    float trans_score = 0.0;
    float dur_score = 0.0;
    float total_score = 0.0;
    while (getline(sinf, line)) {
        stringstream ncl(line);
        ncl >> start >> end >> state_idx;
        int dur = end-start;

        HmmState &state = m_hmm_states[state_idx];
        while (frame_idx < end) {
            m_lna_reader.go_to(frame_idx);
            gmm_score += m_acoustics->log_prob(state_idx);
            trans_score += state.transitions[0].log_prob;
            frame_idx++;
            cerr << frame_idx << "\t" << gmm_score << "\t" << trans_score << "\t" << total_score << endl;
        }
        trans_score += state.transitions[1].log_prob;
        if (duration_model && m_duration_model_in_use)
            dur_score += m_duration_scale * m_hmm_states[state_idx].duration.get_log_prob(dur);
        total_score = gmm_score + trans_score + dur_score;

        //cerr << frame_idx << "\t" << gmm_score << "\t" << trans_score << "\t" << total_score << endl;
    }

    total_score = gmm_score + trans_score + dur_score;
    cerr << "gmm: " << gmm_score << endl;
    cerr << "trans: " << trans_score << endl;
    cerr << "dur: " << dur_score << endl;
    cerr << "total: " << total_score << endl;

    m_lna_reader.close();
    return total_score;
}
