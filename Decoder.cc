#include <algorithm>
#include <cassert>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "NowayHmmReader.hh"
#include "Decoder.hh"
#include "io.hh"

using namespace std;
using namespace fsalm;


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

    NowayHmmReader::read_durations(durf, m_hmms);
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
        double prob = 1.0;
        vector<string> phones;

        string phone;
        stringstream ss(line);
        ss >> unit;
        while (ss >> phone) phones.push_back(phone);

        auto leftp = unit.find("(");
        if (leftp != string::npos) {
            auto rightp = unit.find(")");
            if (rightp == string::npos) throw string("Problem reading line " + linei);
            prob = stod(unit.substr(leftp+1, rightp-leftp-1));
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
    //m_lm.read(io::Stream(lmfname, "r").file);
    m_lm.read_arpa(io::Stream(lmfname, "r").file, true);
    m_lm.trim();
    set_subword_id_fsa_symbol_mapping();
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
        nss >> node_idx >> node.hmm_state >> node.word_id >> arc_count;
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
    add_hmm_self_transitions(m_nodes);
    set_hmm_transition_probs(m_nodes);
    set_word_boundaries();
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
            nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
            nodes[node_idx].arcs.back().target_node = nodes.size()-1;
            node_idx = nodes.size()-1;
        }
        nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
        nodes[node_idx].arcs.back().target_node = START_NODE;
    }
}


void
Decoder::add_hmm_self_transitions(std::vector<Node> &nodes)
{
    for (int i=0; i<nodes.size(); i++) {

        Node &node = nodes[i];
        if (node.hmm_state == -1) continue;

        HmmState &state = m_hmm_states[node.hmm_state];
        node.arcs.insert(node.arcs.begin(), Arc());
        node.arcs[0].log_prob = state.transitions[0].log_prob;
        node.arcs[0].target_node = i;
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
Decoder::recognize_lna_file(string lnafname)
{
    m_lna_reader.open_file(lnafname.c_str(), 1024);
    m_acoustics = &m_lna_reader;
    initialize();

    time_t start_time, end_time;
    time(&start_time);

    float original_global_beam = m_global_beam;
    int frame_idx = 0;
    while (m_lna_reader.go_to(frame_idx)) {
        //bool sil_detected = detect_silence();
        //if (sil_detected && frame_idx > 200) m_global_beam = m_silence_beam;
        if (m_propagated_count > 50000) {
            float beamdiff = min(100.0, ((m_propagated_count-50000.0)/400000.0) * 100.0);
            m_global_beam = original_global_beam-beamdiff;
        }
        else m_global_beam = original_global_beam;
        if (stats) cerr << endl << "recognizing frame: " << frame_idx << endl;
        propagate_tokens();
        prune_tokens();
        if (frame_idx % m_history_prune_frame_interval == 0) prune_word_history();
        frame_idx++;
        if (stats) {
            //if (sil_detected) cerr << "silence_beam was used" << endl;
            cerr << "tokens pruned by global beam: " << m_global_beam_pruned_count << endl;
            cerr << "tokens dropped by max assumption: " << m_dropped_count << endl;
            cerr << "tokens pruned by history beam: " << m_history_beam_pruned_count << endl;
            cerr << "tokens pruned by word end beam: " << m_word_end_beam_pruned_count << endl;
            //cerr << "tokens pruned by history limit: " << m_histogram_pruned_count << endl;
            cerr << "best log probability: " << m_best_log_prob << endl;
            cerr << "worst log probability: " << m_worst_log_prob << endl;
            cerr << "number of active nodes: " << m_tokens.size() << endl;
            cerr << "number of active word histories: " << m_active_histories.size() << endl;
            print_best_word_history();
        }
    }

    time(&end_time);
    double seconds = difftime(end_time, start_time);
    double rtf = seconds / ((double)frame_idx/125.0);
    cerr << "\trecognized " << frame_idx << " frames in " << seconds << " seconds." << endl;
    cerr << "\tRTF: " << rtf << endl;
    cerr << "\tLog prob: " << get_best_token()->total_log_prob << endl;
    cout << lnafname << ":";
    print_best_word_history();

    m_global_beam = original_global_beam;
    clear_word_history();
    m_lna_reader.close();
}


void
Decoder::initialize()
{
    m_word_history_leafs.clear();
    m_tokens.clear();
    m_raw_tokens.clear();
    m_active_histories.clear();
    Token tok;
    tok.fsa_lm_node = m_lm.initial_node_id();
    tok.history = new WordHistory();
    tok.node_idx = DECODE_START_NODE;
    m_tokens[DECODE_START_NODE][tok.history] = tok;
}


void
Decoder::propagate_tokens(void)
{
    m_token_count = 0;
    m_propagated_count = 0;
    m_best_log_prob = -1e20;
    m_best_word_end_prob = -1e20;
    m_worst_log_prob = 0;
    m_global_beam_pruned_count = 0;

    for (auto sit = m_tokens.begin(); sit != m_tokens.end(); ++sit) {
        Node &node = m_nodes[sit->first];
        for (auto hit = sit->second.begin(); hit != sit->second.end(); ++hit) {
            m_token_count++;
            hit->second.word_end = false;
            for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
                move_token_to_node(hit->second, ait->target_node, ait->log_prob);
                m_propagated_count++;
            }
        }
    }

    if (stats) {
        cerr << "token count before propagation: " << m_token_count << endl;
        cerr << "propagated token count: " << m_propagated_count << endl;
    }
}


bool descending_whp_sort(pair<Decoder::WordHistory*, float> i, pair<Decoder::WordHistory*, float> j) { return (i.second > j.second); }

void
Decoder::prune_tokens(void)
{
    m_history_beam_pruned_count = 0;
    m_word_end_beam_pruned_count = 0;
    m_state_beam_pruned_count = 0;
    m_dropped_count = 0;
    vector<Token> pruned_tokens;
    m_active_histories.clear();

    // Global beam pruning
    // Word end beam pruning
    // History beam pruning
    float current_glob_beam = m_best_log_prob - m_global_beam;
    float current_word_end_beam = m_best_word_end_prob - m_word_end_beam;
    for (int i=0; i<m_raw_tokens.size(); i++) {
        Token &tok = m_raw_tokens[i];
        if (tok.total_log_prob < current_glob_beam)
            m_global_beam_pruned_count++;
        else if (tok.total_log_prob < (tok.history->best_token_score-m_history_beam))
            m_history_beam_pruned_count++;
        else if (tok.word_end && (tok.total_log_prob < current_word_end_beam))
            m_word_end_beam_pruned_count++;
        else
            pruned_tokens.push_back(tok);
    }

    // Collect best tokens for each state/history
    m_tokens.clear();
    for (int i=0; i<pruned_tokens.size(); i++) {
        Token &tok = pruned_tokens[i];
        if (tok.total_log_prob > m_tokens[tok.node_idx][tok.history].total_log_prob) {
            m_tokens[tok.node_idx][tok.history] = tok;
            m_active_histories.insert(tok.history);
        }
        else
            m_dropped_count++;
    }

    for (auto histit = m_active_histories.begin(); histit != m_active_histories.end(); ++histit) {
        (*histit)->prune = false;
        (*histit)->best_token_score = -1e20;
    }

    m_raw_tokens.clear();
}


void
Decoder::move_token_to_node(Token token,
                            int node_idx,
                            float transition_score)
{
    if (debug) cerr << "move_token_to_node:" << endl
                    << "\tnode idx: " << node_idx << endl
                    << "\tprevious node idx: " << token.node_idx << endl
                    << "\ttransition score: " << transition_score << endl;

    token.am_log_prob += m_transition_scale * transition_score;

    //token.node_idx = node_idx;
    if (token.node_idx == node_idx) token.dur++;
    else {
        // Apply duration modeling for previous state if moved out from a hmm state
        if (m_nodes[token.node_idx].hmm_state != -1)
            token.am_log_prob += m_duration_scale
                * m_hmm_states[m_nodes[token.node_idx].hmm_state].duration.get_log_prob(token.dur);
        token.node_idx = node_idx;
        token.dur = 1;
    }

    Node &node = m_nodes[node_idx];

    // HMM node
    if (node.hmm_state != -1) {
        token.am_log_prob += m_acoustics->log_prob(node.hmm_state);
        token.total_log_prob = get_token_log_prob(token.am_log_prob, token.lm_log_prob);
        m_best_log_prob = max(m_best_log_prob, token.total_log_prob);
        m_worst_log_prob = min(m_worst_log_prob, token.total_log_prob);
        token.history->best_token_score = max(token.total_log_prob, token.history->best_token_score);
        if (token.word_end) m_best_word_end_prob = max(m_best_word_end_prob, token.total_log_prob);
        m_raw_tokens.push_back(token);
        return;
    }

    // LM node, update LM score
    if (node.word_id >= 0) {
        if (debug) cerr << "node: " << node_idx << " walking with: " << m_subwords[node.word_id] << endl;
        token.fsa_lm_node = m_lm.walk(token.fsa_lm_node, m_subword_id_to_fsa_symbol[node.word_id], &token.lm_log_prob);
        if (node.word_id == SENTENCE_END_WORD_ID) token.fsa_lm_node = m_lm.initial_node_id();
        token.total_log_prob = get_token_log_prob(token.am_log_prob, token.lm_log_prob);
        token.word_end = true;
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }
        token.word_count++;
    }
    else if (node.word_id == -2)
        token.lm_log_prob += m_word_boundary_penalty;

    // LM nodes and word boundaries (-2), update history
    if (node.word_id != -1) {
        auto next_history = token.history->next.find(node.word_id);
        if (next_history != token.history->next.end())
            token.history = next_history->second;
        else {
            token.history = new WordHistory(node.word_id, token.history);
            token.history->previous->next[node.word_id] = token.history;
            m_word_history_leafs.erase(token.history->previous);
            m_word_history_leafs.insert(token.history);
        }
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            move_token_to_node(token, ait->target_node, ait->log_prob);
}


Decoder::Token*
Decoder::get_best_token()
{
    Token *best_token = nullptr;

    for (auto sit = m_tokens.begin(); sit != m_tokens.end(); ++sit) {
        Node &node = m_nodes[sit->first];
        for (auto hit = sit->second.begin(); hit != sit->second.end(); ++hit) {
            if (best_token == nullptr)
                best_token = &(hit->second);
            else if (hit->second.total_log_prob > best_token->total_log_prob)
                best_token = &(hit->second);
        }
    }

    return best_token;
}


void
Decoder::print_best_word_history()
{
    print_word_history(get_best_token()->history);
}


void
Decoder::print_word_history(WordHistory *history)
{
    vector<int> subwords;
    while (true) {
        subwords.push_back(history->word_id);
        if (history->previous == nullptr) break;
        history = history->previous;
    }

    for (auto swit = subwords.rbegin(); swit != subwords.rend(); ++swit) {
        if (*swit >= 0) cout << m_subwords[*swit];
        else if (*swit == -2) cout << " ";
    }
    cout << endl;
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
Decoder::clear_word_history()
{
    for (auto whlnit = m_word_history_leafs.begin(); whlnit != m_word_history_leafs.end(); ++whlnit) {
        WordHistory *wh = *whlnit;
        WordHistory *tmp;
        while (wh != nullptr) {
            tmp = wh;
            wh = wh->previous;
            if (wh != nullptr) wh->next.erase(tmp->word_id);
            delete tmp;
            if (wh != nullptr && wh->next.size() > 0) break;
        }
    }
    m_word_history_leafs.clear();
}


void
Decoder::prune_word_history()
{
    for (auto whlnit = m_word_history_leafs.begin(); whlnit != m_word_history_leafs.end(); ) {
        if (m_active_histories.find(*whlnit) == m_active_histories.end()) {
            WordHistory *wh = *whlnit;
            WordHistory *tmp;
            while (m_active_histories.find(wh) == m_active_histories.end()) {
                tmp = wh;
                wh = wh->previous;
                if (wh != nullptr) wh->next.erase(tmp->word_id);
                delete tmp;
                if (wh != nullptr || wh->next.size() > 0) break;
            }
            m_word_history_leafs.erase(whlnit++);
        }
        else ++whlnit;
    }
}


bool descending_idx_sort(pair<int, float> i, pair<int, float> j) { return (i.second > j.second); }

bool
Decoder::detect_silence()
{
    vector<pair<int, float> > sorted_hmm_states;
    for (int i=0; i<m_hmm_states.size(); i++) {
        float hmm_state_lp = m_acoustics->log_prob(i);
        sorted_hmm_states.push_back(make_pair(i, hmm_state_lp));
    }
    sort(sorted_hmm_states.begin(), sorted_hmm_states.end(), descending_idx_sort);

    int short_sil_pos = 0;
    int long_sil_state_1_pos = 0;
    int long_sil_state_2_pos = 0;
    int long_sil_state_3_pos = 0;
    for (int i=0; i<m_hmm_states.size(); i++) {
        if (sorted_hmm_states[i].first == 0)
            short_sil_pos = i;
        if (sorted_hmm_states[i].first == 1)
            long_sil_state_1_pos = i;
        if (sorted_hmm_states[i].first == 2)
            long_sil_state_2_pos = i;
        if (sorted_hmm_states[i].first == 3)
            long_sil_state_3_pos = i;
    }

    float average_long_sil_rank = (long_sil_state_1_pos + long_sil_state_2_pos + long_sil_state_3_pos) / 3.0;

    m_silence_ranks.push_front(average_long_sil_rank);
    while (m_silence_ranks.size() > 10) m_silence_ranks.pop_back();

    float sliding_rank = 0.0;
    for (auto it = m_silence_ranks.begin(); it != m_silence_ranks.end(); ++it) {
        sliding_rank += *it;
    }
    sliding_rank /= m_silence_ranks.size();

    if (average_long_sil_rank < 200.0 && sliding_rank < 200.0) return true;
    else return false;
}


void
Decoder::collect_fan_in_connections(set<int> &indices,
                                    int node_idx,
                                    int depth)
{
    Node &node = m_nodes[node_idx];
    if (depth == 4) {
        indices.insert(node_idx);
        return;
    }

    depth++;
    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
        collect_fan_in_connections(indices, ait->target_node, depth);
}


void
Decoder::set_word_boundaries()
{
    set<int> fan_in_indices;
    collect_fan_in_connections(fan_in_indices);

    int wb_count = 0;
    for (auto nit = m_nodes.begin(); nit != m_nodes.end(); ++nit) {
        if (nit->hmm_state == -1 && nit->word_id == -1) {
            for (auto ait = nit->arcs.begin(); ait != nit->arcs.end(); ++ait) {
                if (fan_in_indices.find(ait->target_node) != fan_in_indices.end()) {
                    nit->word_id = WORD_BOUNDARY_IDENTIFIER;
                    wb_count++;
                    break;
                }
            }
        }
    }

    m_nodes[START_NODE].word_id = WORD_BOUNDARY_IDENTIFIER;
}

