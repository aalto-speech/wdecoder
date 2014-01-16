#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "NowayHmmReader.hh"
#include "DecoderGraph.hh"

using namespace std;


void
DecoderGraph::read_phone_model(string phnfname)
{
    ifstream phnf(phnfname);
    if (!phnf) throw string("Problem opening phone model.");

    int modelcount;
    NowayHmmReader::read(phnf, m_hmms, m_hmm_map, m_hmm_states, modelcount);
}


void
DecoderGraph::read_duration_model(string durfname)
{
    ifstream durf(durfname);
    if (!durf) throw string("Problem opening duration model.");

    NowayHmmReader::read_durations(durf, m_hmms);
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

        if (m_unit_map.find(unit) == m_unit_map.end()) {
            m_units.push_back(unit);
            m_unit_map[unit] = m_units.size()-1;
        }
        m_lexicon[unit] = phones;

        linei++;
    }
}


void
DecoderGraph::read_word_segmentations(string segfname)
{
    ifstream segf(segfname);
    if (!segf) throw string("Problem opening word segmentations.");

    string line;
    int linei = 1;
    while (getline(segf, line)) {
        string word;
        string subword;

        stringstream ss(line);
        ss >> word;
        if (m_word_segs.find(word) != m_word_segs.end()) throw "Error, segmentation already defined";
        string concatenated;
        while (ss >> subword) {
            if (m_unit_map.find(subword) == m_unit_map.end())
                throw "Subword " + subword + " not found in lexicon";
            m_word_segs[word].push_back(subword);
            concatenated += subword;
        }
        if (concatenated != word) throw "Erroneous segmentation: " + concatenated;

        linei++;
    }
}


void
DecoderGraph::create_word_graph(vector<SubwordNode> &nodes)
{
    nodes.resize(2);
    nodes[START_NODE].subword_id = -1; nodes[END_NODE].subword_id = -1;

    for (auto wit = m_word_segs.begin(); wit != m_word_segs.end(); ++wit) {

        int curr_nd = START_NODE;

        for (int swidx = 0; swidx < wit->second.size(); ++swidx) {

            string curr_subword = (wit->second)[swidx];
            int curr_subword_idx = m_unit_map[curr_subword];

            if (nodes[curr_nd].out_arcs.find(curr_subword_idx) == nodes[curr_nd].out_arcs.end()) {
                nodes.resize(nodes.size()+1);
                nodes.back().subword_id = curr_subword_idx;
                nodes[curr_nd].out_arcs[curr_subword_idx] = nodes.size()-1;
                nodes.back().in_arcs.push_back(make_pair(nodes[curr_nd].subword_id, curr_nd));
                curr_nd = nodes.size()-1;
            }
            else
                curr_nd = nodes[curr_nd].out_arcs[curr_subword_idx];
        }

        // Connect to end node
        nodes[curr_nd].out_arcs[-1] = END_NODE;
        nodes[END_NODE].in_arcs.push_back(make_pair(nodes[curr_nd].subword_id, curr_nd));
    }
}


void
DecoderGraph::tie_word_graph_suffixes(vector<SubwordNode> &nodes)
{
    map<int, int> suffix_counts;
    SubwordNode &end_node = nodes[END_NODE];

    for (auto sit = end_node.in_arcs.begin(); sit != end_node.in_arcs.end(); ++sit)
        suffix_counts[sit->first] += 1;

    for (auto sit = suffix_counts.begin(); sit != suffix_counts.end(); ++sit) {
        if (sit->second > 1) {
            nodes.resize(nodes.size()+1);
            nodes.back().subword_id = sit->first;
            nodes.back().out_arcs[-1] = END_NODE;
            for (auto ait = end_node.in_arcs.begin(); ait != end_node.in_arcs.end(); ++ait) {
                if (ait->first == sit->first) {
                    int src_node_idx = nodes[ait->second].in_arcs[0].second;
                    nodes[src_node_idx].out_arcs[ait->first] = nodes.size()-1;
                    nodes.back().in_arcs.push_back(make_pair(nodes[src_node_idx].subword_id, src_node_idx));
                    nodes[ait->second].subword_id = -1;
                    nodes[ait->second].in_arcs.clear();
                    nodes[ait->second].out_arcs.clear();
                }
            }
        }
    }
}


void
DecoderGraph::print_word_graph(std::vector<SubwordNode> &nodes,
                               std::vector<int> path,
                               int node_idx)
{
    path.push_back(nodes[node_idx].subword_id);

    for (auto ait = nodes[node_idx].out_arcs.begin(); ait != nodes[node_idx].out_arcs.end(); ++ait) {
        if (ait->second == END_NODE) {
            for (int i=0; i<path.size(); i++) {
                if (path[i] >= 0) cout << m_units[path[i]];
                if (i+1 != path.size()) cout << " ";
            }
            cout << endl;
        }
        else {
            print_word_graph(nodes, path, ait->second);
        }
    }
}


void
DecoderGraph::print_word_graph(std::vector<SubwordNode> &nodes)
{
    std::vector<int> path;
    print_word_graph(nodes, path, START_NODE);
}


void
DecoderGraph::reachable_word_graph_nodes(std::vector<SubwordNode> &nodes,
                                         std::set<int> &node_idxs,
                                         int node_idx)
{
    node_idxs.insert(node_idx);
    for (auto ait = nodes[node_idx].out_arcs.begin(); ait != nodes[node_idx].out_arcs.end(); ++ait)
        reachable_word_graph_nodes(nodes, node_idxs, ait->second);
}


int
DecoderGraph::reachable_word_graph_nodes(std::vector<SubwordNode> &nodes)
{
    std::set<int> node_idxs;
    reachable_word_graph_nodes(nodes, node_idxs, START_NODE);
    return node_idxs.size();
}


void
DecoderGraph::expand_subword_nodes(const std::vector<SubwordNode> &swnodes,
                                   std::vector<Node> &nodes,
                                   int sw_node_idx,
                                   int node_idx,
                                   char left_context,
                                   char second_left_context,
                                   int debug)
{

    if (sw_node_idx == END_NODE) return;

    const SubwordNode &swnode = swnodes[sw_node_idx];
    if (swnode.subword_id == -1) {
        for (auto ait = swnode.out_arcs.begin(); ait != swnode.out_arcs.end(); ++ait)
            if (ait->second != END_NODE)
                expand_subword_nodes(swnodes, nodes, ait->second, node_idx, left_context, second_left_context, debug);
            // FIXME: need to handle end node case ?
        return;
    }

    string subword = m_units[swnode.subword_id];
    if (debug) cerr << subword << "\tleft context: " << left_context << endl;
    auto triphones = m_lexicon[subword];

    // Construct the connecting triphone and expand states
    if (second_left_context != '_' && left_context != '_') {
        nodes.resize(nodes.size()+1);
        nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
        nodes[node_idx].arcs.back().target_node = nodes.size()-1;
        string triphone = string(1, second_left_context) + "-" + string(1,left_context) + "+" + string(1,triphones[0][2]);
        int hmm_index = m_hmm_map[triphone];
        Hmm &hmm = m_hmms[hmm_index];
        for (int sidx = 2; sidx < hmm.states.size(); ++sidx) {
            nodes.resize(nodes.size()+1);
            nodes.back().hmm_state = hmm.states[sidx].model; // FIXME: is this correct idx?
            nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
            nodes[node_idx].arcs.back().target_node = nodes.size()-1;
            node_idx = nodes.size()-1;
        }
    }

    for (int tidx = 0; tidx < triphones.size()-1; ++tidx) {
        string triphone = triphones[tidx];
        if (tidx == 0) triphone = left_context + triphone.substr(1);
        int hmm_index = m_hmm_map[triphone];
        Hmm &hmm = m_hmms[hmm_index];

        if (debug) {
            cerr << "  " << triphone << " (" << triphone[2] << ")" << endl;
            for (int sidx = 2; sidx < hmm.states.size(); ++sidx) {
                cerr << "\t" << hmm.states[sidx].model;
                for (int transidx = 0; transidx<hmm.states[sidx].transitions.size(); ++transidx)
                    cerr << "\ttarget: " << hmm.states[sidx].transitions[transidx].target
                    << " lp: " << hmm.states[sidx].transitions[transidx].log_prob;
                cerr << endl;
            }
            cerr << endl;
        }

        // Connect triphone state nodes
        for (int sidx = 2; sidx < hmm.states.size(); ++sidx) {
            nodes.resize(nodes.size()+1);
            nodes.back().hmm_state = hmm.states[sidx].model;
            nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
            nodes[node_idx].arcs.back().target_node = nodes.size()-1;
            node_idx = nodes.size()-1;
        }
    }
    if (debug) cerr << endl;

    // Connect dummy node with subword id
    nodes.resize(nodes.size()+1);
    nodes.back().word_id = swnode.subword_id;
    nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
    nodes[node_idx].arcs.back().target_node = nodes.size()-1;
    node_idx = nodes.size()-1;

    char last_phone = triphones[triphones.size()-1][2];
    char second_last_phone = left_context;
    if (triphones.size() > 1) second_last_phone = triphones[triphones.size()-2][2];
    int curr_node = nodes.size()-1;
    for (auto ait = swnode.out_arcs.begin(); ait != swnode.out_arcs.end(); ++ait)
        if (ait->second != END_NODE)
            expand_subword_nodes(swnodes, nodes, ait->second, curr_node, last_phone, second_last_phone);
}


int
DecoderGraph::add_lm_unit(string unit)
{
    auto uit = m_unit_map.find(unit);
    if (uit != m_unit_map.end())
        return (*uit).second;

    int index = m_units.size();
    m_unit_map[unit] = index;
    m_units.push_back(unit);

    return index;
}
