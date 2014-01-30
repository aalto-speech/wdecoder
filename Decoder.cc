#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

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

    m_lm.read_arpa(io::Stream(lmfname, "r").file, true);
    m_lm.trim();
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
    }
}


void
Decoder::recognize_lna_file(string &lnafname)
{
    m_lna_reader.open_file(lnafname.c_str(), 1024);
    m_acoustics = &m_lna_reader;
    initialize();

    int frame_idx = 0;
    while (m_lna_reader.go_to(frame_idx)) {
        if (debug) cerr << endl << "recognizing frame: " << frame_idx << endl;
        propagate_tokens();
        frame_idx++;
        if (frame_idx == 2) break;
    }

    m_lna_reader.close();
}


void
Decoder::initialize(void) {
    set_subword_id_fsa_symbol_mapping();
    m_tokens.resize(1);
    m_tokens.back().fsa_lm_node = m_lm.initial_node_id();
    m_tokens.back().history = make_shared<WordHistory>();
    m_tokens.back().history->word_id = -1;
    m_tokens.back().node_idx = END_NODE;
}


void
Decoder::propagate_tokens(void)
{
    m_best_log_prob = -1e20;
    m_worst_log_prob = 0;

    if (debug) cerr << "number of tokens: " << m_tokens.size() << endl;
    std::vector<Token> tokens;
    m_tokens.swap(tokens);

    int token_idx = 0;
    for (auto token = tokens.begin(); token != tokens.end(); ++token) {
        if (debug) cerr << "processing token: " << token_idx << endl;
        Node &nd = m_nodes[token->node_idx];
        if (debug) cerr << "number of arcs: " << nd.arcs.size() << endl;
        for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait)
            move_token_to_node(*token, ait->target_node, ait->log_prob);
        token_idx++;
    }
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

    // LM node, update history and LM score
    if (node.word_id != -1) {
        token.fsa_lm_node = m_lm.walk(token.fsa_lm_node, m_subword_id_to_fsa_symbol[node.word_id], &token.lm_log_prob);
        token.total_log_prob = get_token_log_prob(token.am_log_prob, token.lm_log_prob);

        if (token.history->next.find(node.word_id) == token.history->next.end())
            token.history = make_shared<WordHistory>(node.word_id, token.history);
        else
            token.history = token.history->next[node.word_id].lock();
    }

    if (node.hmm_state == -1) {
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            move_token_to_node(token, ait->target_node, ait->log_prob);
        return;
    }

    // HMM node
    if (node.hmm_state != -1) {
        token.am_log_prob += m_acoustics->log_prob(node.hmm_state);
        token.total_log_prob = get_token_log_prob(token.am_log_prob, token.lm_log_prob);
    }

    m_tokens.push_back(token);
}

