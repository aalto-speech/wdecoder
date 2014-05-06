#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "NowayHmmReader.hh"
#include "DecoderGraph.hh"
#include "gutils.hh"

using namespace std;
using namespace gutils;


void
DecoderGraph::read_phone_model(string phnfname)
{
    ifstream phnf(phnfname);
    if (!phnf) throw string("Problem opening phone model.");

    int modelcount;
    NowayHmmReader::read(phnf, m_hmms, m_hmm_map, m_hmm_states, modelcount);
}


void
DecoderGraph::read_noway_lexicon(string lexfname)
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
            double prob = ::atof(unit.substr(leftp+1, rightp-leftp-1).c_str());
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
DecoderGraph::collect_cw_fanout_nodes(vector<Node> &nodes,
                                      map<int, string> &nodes_to_fanout,
                                      int hmm_state_count,
                                      vector<char> phones,
                                      int node_to_connect,
                                      int node_idx)
{
    if (node_idx == START_NODE) return;
    Node &node = nodes[node_idx];

    if (node.hmm_state != -1) hmm_state_count++;
    if (node.word_id != -1) {
        string &subword = m_subwords[node.word_id];
        vector<string> &triphones = m_lexicon[subword];
        int tri_idx = triphones.size()-1;
        while (phones.size() < 2 && tri_idx >= 0) {
            phones.push_back(triphones[tri_idx][2]);
            tri_idx--;
        }
    }

    if (node_to_connect == -1 &&
        (hmm_state_count == 4 || (hmm_state_count == 3 && node.word_id != -1))) node_to_connect = node_idx;

    if (phones.size() == 2 && hmm_state_count > 2) {
        string triphone = string(1,phones[1]) + string(1,'-') + string(1,phones[0]) + string("+_");
        if (nodes_to_fanout.find(node_to_connect) == nodes_to_fanout.end()) {
            nodes_to_fanout[node_to_connect] = triphone;
            if (debug) cerr << "Connecting node " << node_to_connect
                            << " to fanout with triphone " << triphone << endl;
        }
        else
            assert(nodes_to_fanout[node_to_connect] == triphone);
        return;
    }

    for (auto ait = node.reverse_arcs.begin(); ait != node.reverse_arcs.end(); ++ait)
        collect_cw_fanout_nodes(nodes, nodes_to_fanout, hmm_state_count,
                                phones, node_to_connect, *ait);
}


void
DecoderGraph::collect_cw_fanin_nodes(vector<Node> &nodes,
                                     map<node_idx_t, string> &nodes_from_fanin,
                                     int hmm_state_count,
                                     vector<char> phones,
                                     node_idx_t node_to_connect,
                                     node_idx_t node_idx)
{
    if (node_idx == END_NODE) return;
    Node &node = nodes[node_idx];

    if (debug) cerr << endl << "in node: " << node_idx;

    if (node.hmm_state != -1) hmm_state_count++;
    if (node.word_id != -1) {
        string &subword = m_subwords[node.word_id];
        vector<string> &triphones = m_lexicon[subword];
        unsigned int tri_idx = 0;
        while (phones.size() < 2 && tri_idx < triphones.size()) {
            phones.push_back(triphones[tri_idx][2]);
            tri_idx++;
        }
    }

    if (node_to_connect == START_NODE &&
        (hmm_state_count == 4 || (hmm_state_count == 3 && node.word_id != -1))) node_to_connect = node_idx;

    if (phones.size() == 2 && hmm_state_count > 2) {
        string triphone = string("_-") + string(1,phones[0]) + string(1,'+') + string(1,phones[1]);
        if (nodes_from_fanin.find(node_to_connect) == nodes_from_fanin.end()) {
            nodes_from_fanin[node_to_connect] = triphone;
            if (debug) cerr << endl << "Connecting node " << node_to_connect
                            << " from fanin with triphone " << triphone << endl;
        }
        else {
            assert(nodes_from_fanin[node_to_connect] == triphone);
        }
        return;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (debug) cerr << endl <<  "proceeding to node " << *ait
                        << " from node: " << node_idx;
        collect_cw_fanin_nodes(nodes, nodes_from_fanin, hmm_state_count,
                               phones, node_to_connect, *ait);
    }
}



