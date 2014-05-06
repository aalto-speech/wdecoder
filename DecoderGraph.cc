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
DecoderGraph::connect_crossword_network(vector<Node> &nodes,
                                        vector<Node> &cw_nodes,
                                        map<string, int> &fanout,
                                        map<string, int> &fanin,
                                        bool push_left_after_fanin)
{
    int offset = nodes.size();
    for (auto cwnit = cw_nodes.begin(); cwnit != cw_nodes.end(); ++cwnit) {
        nodes.push_back(*cwnit);
        set<unsigned int> temp_arcs = nodes.back().arcs;
        nodes.back().arcs.clear();
        for (auto ait = temp_arcs.begin(); ait != temp_arcs.end(); ++ait)
            nodes.back().arcs.insert(*ait + offset);
    }

    for (auto fonit = fanout.begin(); fonit != fanout.end(); ++fonit)
        fonit->second += offset;
    for (auto finit = fanin.begin(); finit != fanin.end(); ++finit)
        finit->second += offset;

    map<node_idx_t, string> nodes_to_fanin;
    collect_cw_fanin_nodes(nodes, nodes_to_fanin);

    for (auto finit = nodes_to_fanin.begin(); finit != nodes_to_fanin.end(); ++finit) {
        if (fanin.find(finit->second) == fanin.end()) {
            cerr << "Problem, triphone: " << finit->second << " not found in fanin" << endl;
            assert(false);
        }
        int fanin_idx = fanin[finit->second];
        if (debug) cerr << "connecting node " << finit->first
                        << " from fanin node " << fanin_idx << " with triphone " << finit->second << endl;
        Node &fanin_node = nodes[fanin_idx];
        int node_idx = finit->first;
        fanin_node.arcs.insert(node_idx);
    }

    if (push_left_after_fanin)
        push_word_ids_left(nodes);
    else
        set_reverse_arcs_also_from_unreachable(nodes);

    map<int, string> nodes_to_fanout;
    collect_cw_fanout_nodes(nodes, nodes_to_fanout);

    for (auto fonit = nodes_to_fanout.begin(); fonit != nodes_to_fanout.end(); ++fonit) {
        if (fanout.find(fonit->second) == fanout.end()) {
            cerr << "Problem, triphone: " << fonit->second << " not found in fanout" << endl;
            assert(false);
        }
        int fanout_idx = fanout[fonit->second];
        if (debug) cerr << "connecting node " << fonit->first
                        << " to fanout node " << fanout_idx << " with triphone " << fonit->second << endl;
        Node &node = nodes[fonit->first];
        node.arcs.insert(fanout_idx);
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


void
DecoderGraph::add_word(std::vector<TriphoneNode> &nodes,
                       std::string word,
                       std::vector<std::string> &triphones)
{
    vector<string> &subwords = m_word_segs.at(word);
    vector<int> subword_ids;
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit)
        subword_ids.push_back(m_subword_map[*swit]);

    vector<int> hmm_ids;
    for (auto trit = triphones.begin(); trit != triphones.end(); ++trit) {
        int hmm_index = m_hmm_map[*trit];
        hmm_ids.push_back(hmm_index);
    }

    int lm_label_pos = min(hmm_ids.size()-1, m_lexicon[(*(subwords.begin()))].size()-1);
    lm_label_pos = max(1, lm_label_pos);

    int curr_node_idx = START_NODE;
    for (int i=0; i<lm_label_pos; i++) {
        int hmm_state_id = hmm_ids[i];
        if (nodes[curr_node_idx].hmm_id_lookahead.find(hmm_state_id) != nodes[curr_node_idx].hmm_id_lookahead.end()) {
            curr_node_idx = nodes[curr_node_idx].hmm_id_lookahead[hmm_state_id];
        }
        else {
            nodes.resize(nodes.size()+1);
            nodes.back().hmm_id = hmm_state_id;
            nodes[curr_node_idx].hmm_id_lookahead[hmm_state_id] = nodes.size()-1;
            curr_node_idx = nodes.size()-1;
        }
    }

    for (unsigned int i=0; i<subword_ids.size(); i++) {
        int subword_id = subword_ids[i];
        if (nodes[curr_node_idx].subword_id_lookahead.find(subword_id) != nodes[curr_node_idx].subword_id_lookahead.end()) {
            curr_node_idx = nodes[curr_node_idx].subword_id_lookahead[subword_id];
        }
        else {
            nodes.resize(nodes.size()+1);
            nodes.back().subword_id = subword_id;
            nodes[curr_node_idx].subword_id_lookahead[subword_id] = nodes.size()-1;
            curr_node_idx = nodes.size()-1;
        }
    }

    for (unsigned int i=lm_label_pos; i<hmm_ids.size(); i++) {
        int hmm_state_id = hmm_ids[i];
        if (nodes[curr_node_idx].hmm_id_lookahead.find(hmm_state_id) != nodes[curr_node_idx].hmm_id_lookahead.end()) {
            curr_node_idx = nodes[curr_node_idx].hmm_id_lookahead[hmm_state_id];
        }
        else {
            nodes.resize(nodes.size()+1);
            nodes.back().hmm_id = hmm_state_id;
            nodes[curr_node_idx].hmm_id_lookahead[hmm_state_id] = nodes.size()-1;
            curr_node_idx = nodes.size()-1;
        }
    }

    nodes[curr_node_idx].connect_to_end_node = true;
}


void
DecoderGraph::add_triphones(std::vector<TriphoneNode> &nodes,
                            std::vector<TriphoneNode> &nodes_to_add)
{
    int curr_node_idx = START_NODE;

    for (auto ntait = nodes_to_add.begin(); ntait != nodes_to_add.end(); ++ntait) {
        if (ntait->hmm_id != -1) {
            int hmm_id = ntait->hmm_id;
            if (nodes[curr_node_idx].hmm_id_lookahead.find(hmm_id) != nodes[curr_node_idx].hmm_id_lookahead.end()) {
                curr_node_idx = nodes[curr_node_idx].hmm_id_lookahead[hmm_id];
            }
            else {
                nodes.resize(nodes.size()+1);
                nodes.back().hmm_id = hmm_id;
                nodes[curr_node_idx].hmm_id_lookahead[hmm_id] = nodes.size()-1;
                curr_node_idx = nodes.size()-1;
            }
        }
        else if (ntait->subword_id != -1) {
            int subword_id = ntait->subword_id;
            if (nodes[curr_node_idx].subword_id_lookahead.find(subword_id) != nodes[curr_node_idx].subword_id_lookahead.end()) {
                curr_node_idx = nodes[curr_node_idx].subword_id_lookahead[subword_id];
            }
            else {
                nodes.resize(nodes.size()+1);
                nodes.back().subword_id = subword_id;
                nodes[curr_node_idx].subword_id_lookahead[subword_id] = nodes.size()-1;
                curr_node_idx = nodes.size()-1;
            }
        }
    }

    nodes[curr_node_idx].connect_to_end_node = true;
}


void
DecoderGraph::triphones_to_states(std::vector<TriphoneNode> &triphone_nodes,
                                  std::vector<Node> &nodes,
                                  int curr_tri_idx,
                                  int curr_state_idx)
{
    if (triphone_nodes[curr_tri_idx].connect_to_end_node)
        nodes[curr_state_idx].arcs.insert(END_NODE);

    for (auto triit = triphone_nodes[curr_tri_idx].hmm_id_lookahead.begin();
         triit != triphone_nodes[curr_tri_idx].hmm_id_lookahead.end();
         ++triit)
    {
        int new_state_idx = connect_triphone(nodes, triit->first, curr_state_idx);
        triphones_to_states(triphone_nodes, nodes, triit->second, new_state_idx);
    }

    for (auto swit = triphone_nodes[curr_tri_idx].subword_id_lookahead.begin();
         swit != triphone_nodes[curr_tri_idx].subword_id_lookahead.end();
         ++swit)
    {
        nodes.resize(nodes.size()+1);
        nodes.back().word_id = swit->first;
        nodes[curr_state_idx].arcs.insert(nodes.size()-1);
        triphones_to_states(triphone_nodes, nodes, swit->second, nodes.size()-1);
    }
}


