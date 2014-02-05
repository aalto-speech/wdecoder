#include <algorithm>
#include <cassert>
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

    for (auto wit = m_word_segs.begin(); wit != m_word_segs.end(); ++wit) {

        vector<vector<int> > word_sw_nodes;
        bool tie_first = false;
        for (int swidx = 0; swidx < wit->second.size(); ++swidx) {

            string curr_subword = (wit->second)[swidx];
            if (swidx == 0 && m_lexicon[curr_subword].size() == 1) {
                tie_first = true;
            }
            else if (swidx == 1 && tie_first) {
                vector<int> tmp;
                int subword_idx = m_unit_map[(wit->second)[0]];
                tmp.push_back(subword_idx);
                subword_idx = m_unit_map[curr_subword];
                tmp.push_back(subword_idx);
                word_sw_nodes.push_back(tmp);
            }
            else if (swidx == (wit->second.size()-1) && m_lexicon[curr_subword].size() == 1) {
                int curr_subword_idx = m_unit_map[curr_subword];
                word_sw_nodes.back().push_back(curr_subword_idx);
            }
            else {
                vector<int> tmp;
                int subword_idx = m_unit_map[curr_subword];
                tmp.push_back(subword_idx);
                word_sw_nodes.push_back(tmp);
            }
        }

        int curr_nd = START_NODE;
        for (int swnidx = 0; swnidx < word_sw_nodes.size(); ++swnidx) {

            if (nodes[curr_nd].out_arcs.find(word_sw_nodes[swnidx]) == nodes[curr_nd].out_arcs.end()) {
                nodes.resize(nodes.size()+1);
                nodes.back().subword_ids = word_sw_nodes[swnidx];
                nodes[curr_nd].out_arcs[word_sw_nodes[swnidx]] = nodes.size()-1;
                nodes.back().in_arcs.push_back(make_pair(nodes[curr_nd].subword_ids, curr_nd));
                curr_nd = nodes.size()-1;
            }
            else
                curr_nd = nodes[curr_nd].out_arcs[word_sw_nodes[swnidx]];
        }

        // Connect to end node
        vector<int> empty;
        nodes[curr_nd].out_arcs[empty] = END_NODE;
        nodes[END_NODE].in_arcs.push_back(make_pair(nodes[curr_nd].subword_ids, curr_nd));
    }
}


void
DecoderGraph::tie_subword_suffixes(vector<SubwordNode> &nodes)
{
    map<vector<int>, int> suffix_counts;
    SubwordNode &node = nodes[END_NODE];

    for (auto sit = node.in_arcs.begin(); sit != node.in_arcs.end(); ++sit)
        suffix_counts[sit->first] += 1;

    for (auto sit = suffix_counts.begin(); sit != suffix_counts.end(); ++sit) {
        if (sit->second > 1) {
            nodes.resize(nodes.size()+1);
            nodes.back().subword_ids = sit->first;
            vector<int> empty;
            nodes.back().out_arcs[empty] = END_NODE;
            for (auto ait = node.in_arcs.begin(); ait != node.in_arcs.end(); ++ait) {
                if (ait->first == sit->first) {
                    int src_node_idx = nodes[ait->second].in_arcs[0].second;
                    nodes[src_node_idx].out_arcs[ait->first] = nodes.size()-1;
                    nodes.back().in_arcs.push_back(make_pair(nodes[src_node_idx].subword_ids, src_node_idx));
                    nodes[ait->second].subword_ids.clear();
                    nodes[ait->second].in_arcs.clear();
                    nodes[ait->second].out_arcs.clear();
                }
            }
        }
    }
}


void
DecoderGraph::print_word_graph(vector<SubwordNode> &nodes,
                               vector<int> path,
                               int node_idx)
{
    path.push_back(node_idx);

    /*
    for (auto ait = nodes[node_idx].out_arcs.begin(); ait != nodes[node_idx].out_arcs.end(); ++ait) {
        if (ait->second == END_NODE) {
            for (int i=0; i<path.size(); i++) {
                int subword_id = nodes[path[i]].subword_id;
                if (subword_id == -1) continue;
                if (path[i] >= 0) cout << m_units[subword_id] << " (" << path[i] << ")";
                if (i+1 != path.size()) cout << " ";
            }
            cout << endl;
        }
        else {
            print_word_graph(nodes, path, ait->second);
        }
    }
    */
}


void
DecoderGraph::print_word_graph(vector<SubwordNode> &nodes)
{
    vector<int> path;
    print_word_graph(nodes, path, START_NODE);
}


void
DecoderGraph::reachable_word_graph_nodes(vector<SubwordNode> &nodes,
                                         set<int> &node_idxs,
                                         int node_idx)
{
    node_idxs.insert(node_idx);
    for (auto ait = nodes[node_idx].out_arcs.begin(); ait != nodes[node_idx].out_arcs.end(); ++ait)
        reachable_word_graph_nodes(nodes, node_idxs, ait->second);
}


int
DecoderGraph::reachable_word_graph_nodes(vector<SubwordNode> &nodes)
{
    set<int> node_idxs;
    reachable_word_graph_nodes(nodes, node_idxs, START_NODE);
    return node_idxs.size();
}


void
DecoderGraph::expand_subword_nodes(const vector<SubwordNode> &swnodes,
                                   vector<Node> &nodes,
                                   int sw_node_idx,
                                   int node_idx,
                                   char left_context,
                                   char second_left_context,
                                   int delayed_subword_id)
{
    /*
    if (sw_node_idx == START_NODE) nodes.resize(2);
    if (sw_node_idx == END_NODE) return;

    const SubwordNode &swnode = swnodes[sw_node_idx];
    if (swnode.subword_id == -1) {
        for (auto ait = swnode.out_arcs.begin(); ait != swnode.out_arcs.end(); ++ait)
            if (ait->second != END_NODE)
                expand_subword_nodes(swnodes, nodes, ait->second,
                                     node_idx, left_context, second_left_context);
        return;
    }

    string subword = m_units[swnode.subword_id];
    if (debug) cerr << endl << subword
                    <<  "\tsecond left context: " << second_left_context
                    << "\tleft context: " << left_context << endl;;
    if (debug && delayed_subword_id != -1) cerr << "delayed subword: " << m_units[delayed_subword_id] <<endl;

    auto triphones = m_lexicon[subword];

    // One phone subword in the beginning of word, delay expansion
    if (triphones.size() == 1 && second_left_context == '_' && left_context == '_') {
        if (debug) cerr << "..delay expansion" << endl;
        second_left_context = '_';
        left_context = triphones[0][2];
        for (auto ait = swnode.out_arcs.begin(); ait != swnode.out_arcs.end(); ++ait)
            if (ait->second != END_NODE)
                expand_subword_nodes(swnodes, nodes, ait->second, node_idx,
                                     left_context, second_left_context, swnode.subword_id);
        return;
    }

    if (delayed_subword_id != -1) {
        string triphone = string(1,second_left_context) + "-" + string(1,left_context) + "+" + triphones[0][2];
        node_idx = connect_triphone(nodes, triphone, node_idx);
    }

    // Construct the left connecting triphone and expand states
    if (second_left_context != '_' && left_context != '_') {
        string triphone = string(1,second_left_context) + "-" + string(1,left_context) + "+" + string(1,triphones[0][2]);
        node_idx = connect_triphone(nodes, triphone, node_idx);
    }

    if (triphones.size() > 1) {
        string triphone = string(1,left_context) + triphones[0].substr(1);
        node_idx = connect_triphone(nodes, triphone, node_idx);
    }

    for (int tidx = 1; tidx < triphones.size()-1; ++tidx) {
        string triphone = triphones[tidx];
        node_idx = connect_triphone(nodes, triphone, node_idx);
    }


    char last_phone = triphones[triphones.size()-1][2];
    char second_last_phone = left_context;
    if (triphones.size() > 1) second_last_phone = triphones[triphones.size()-2][2];

    if (swnode.out_arcs.size() == 1 && swnode.out_arcs.begin()->second == END_NODE) {
        if (debug) cerr << "connecting to end node" << endl;
        string triphone = string(1,second_last_phone) + "-" + string(1,last_phone) + "+_";
        int temp_node_idx = connect_triphone(nodes, triphone, node_idx);

        if (delayed_subword_id != -1) {
            nodes.resize(nodes.size()+1);
            nodes.back().word_id = delayed_subword_id;
            nodes[temp_node_idx].arcs.resize(nodes[temp_node_idx].arcs.size()+1);
            nodes[temp_node_idx].arcs.back().target_node = nodes.size()-1;
            temp_node_idx = nodes.size()-1;
        }

        nodes.resize(nodes.size()+1);
        nodes.back().word_id = swnode.subword_id;
        nodes[temp_node_idx].arcs.resize(nodes[temp_node_idx].arcs.size()+1);
        nodes[temp_node_idx].arcs.back().target_node = nodes.size()-1;
        temp_node_idx = nodes.size()-1;

        nodes[temp_node_idx].arcs.resize(nodes[temp_node_idx].arcs.size()+1);
        nodes[temp_node_idx].arcs.back().target_node = END_NODE;

        return;
    }


    if (debug) cerr << endl;
    if (debug) cerr << "adding dummy node for subword: " << m_units[swnode.subword_id]
                    << " subword node idx: " << sw_node_idx << endl;

    if (delayed_subword_id != -1) {
        nodes.resize(nodes.size()+1);
        nodes.back().word_id = delayed_subword_id;
        nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
        nodes[node_idx].arcs.back().target_node = nodes.size()-1;
        node_idx = nodes.size()-1;
    }

    nodes.resize(nodes.size()+1);
    nodes.back().word_id = swnode.subword_id;
    nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
    nodes[node_idx].arcs.back().target_node = nodes.size()-1;
    node_idx = nodes.size()-1;

    for (auto ait = swnode.out_arcs.begin(); ait != swnode.out_arcs.end(); ++ait) {

        if (ait->second == END_NODE) {
            string triphone = string(1,second_last_phone) + "-" + string(1,last_phone) + "+_";
            int temp_node_idx = connect_triphone(nodes, triphone, node_idx);
            nodes[temp_node_idx].arcs.resize(nodes[temp_node_idx].arcs.size()+1);
            nodes[temp_node_idx].arcs.back().target_node = END_NODE;
        }
        else
            expand_subword_nodes(swnodes, nodes, ait->second, node_idx,
                                 last_phone, second_last_phone);
    }
    */
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


int
DecoderGraph::connect_triphone(vector<DecoderGraph::Node> &nodes,
                               string triphone,
                               int node_idx)
{
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
    }

    for (int sidx = 2; sidx < hmm.states.size(); ++sidx) {
        nodes.resize(nodes.size()+1);
        nodes.back().hmm_state = hmm.states[sidx].model; // FIXME: is this correct idx?
        nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
        nodes[node_idx].arcs.back().target_node = nodes.size()-1;
        node_idx = nodes.size()-1;
    }

    return node_idx;
}


void
DecoderGraph::print_graph(vector<Node> &nodes,
                          vector<int> path,
                          int node_idx)
{
    path.push_back(node_idx);

    if (node_idx == END_NODE) {
        for (int i=0; i<path.size(); i++) {
            if (nodes[path[i]].hmm_state != -1)
                //cout << " " << nodes[path[i]].hmm_state;
                cout << " " << path[i] << "(" << nodes[path[i]].hmm_state << ")";
            if (nodes[path[i]].word_id != -1)
                //cout << " " << "(" << m_units[nodes[path[i]].word_id] << ")";
                cout << " " << path[i] << "(" << m_units[nodes[path[i]].word_id] << ")";
        }
        cout << endl << endl;
        return;
    }

    for (auto ait = nodes[node_idx].arcs.begin(); ait != nodes[node_idx].arcs.end(); ++ait)
        print_graph(nodes, path, ait->target_node);
}


void
DecoderGraph::print_graph(vector<Node> &nodes)
{
    vector<int> path;
    print_graph(nodes, path, START_NODE);
}


void
DecoderGraph::tie_state_prefixes(vector<Node> &nodes,
                                 bool stop_propagation)
{
    set<int> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);
    tie_state_prefixes(nodes, processed_nodes, stop_propagation, START_NODE);
}


void
DecoderGraph::tie_state_prefixes(vector<Node> &nodes,
                                 set<int> &processed_nodes,
                                 bool stop_propagation,
                                 int node_idx)
{
    if (node_idx == END_NODE) return;
    if (processed_nodes.find(node_idx) != processed_nodes.end()) return;
    if (debug) cerr << endl << "tying state: " << node_idx << endl;
    processed_nodes.insert(node_idx);
    Node &nd = nodes[node_idx];

    map<int, set<int> > targets;
    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait) {
        int target_hmm = nodes[ait->target_node].hmm_state;
        if (target_hmm != -1) targets[target_hmm].insert(ait->target_node);
    }

    if (debug) {
        cerr << "total targets: " << targets.size() << endl;
        for (auto tit = targets.begin(); tit !=targets.end(); ++tit) {
            cerr << "hmm state: " << tit->first << " number of targets: " << tit->second.size() << endl;
            cerr << "targets:";
            for (auto nit = tit->second.begin(); nit != tit->second.end(); ++nit)
                cerr << " node: " << *nit << " ";
            cerr << endl;
        }
    }

    bool arcs_removed = false;
    for (auto tit = targets.begin(); tit !=targets.end(); ++tit) {
        if (tit->second.size() == 1) continue;

        auto nit = tit->second.begin();
        int tied_node_idx = *nit;
        nit++;
        while (nit != tit->second.end()) {
            int curr_node_idx = *nit;
            tied_node_idx = merge_nodes(nodes, tied_node_idx, curr_node_idx);
            arcs_removed = true;
            nit++;
        }

        if (debug) cerr << "\tnew tied node idx: " << tied_node_idx << endl;
    }

    if (stop_propagation && !arcs_removed) return;

    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait)
        tie_state_prefixes(nodes, processed_nodes, stop_propagation, ait->target_node);
}


void
DecoderGraph::set_reverse_arcs_also_from_unreachable(vector<Node> &nodes)
{
    clear_reverse_arcs(nodes);
    for (int i = 0; i < nodes.size(); ++i) {
        Node &node = nodes[i];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            nodes[ait->target_node].reverse_arcs.resize(nodes[ait->target_node].reverse_arcs.size()+1);
            nodes[ait->target_node].reverse_arcs.back().target_node = i;
        }
    }
}


void
DecoderGraph::set_reverse_arcs(vector<Node> &nodes)
{

    clear_reverse_arcs(nodes);
    set<int> processed_nodes;
    set_reverse_arcs(nodes, START_NODE, processed_nodes);
}


void
DecoderGraph::set_reverse_arcs(vector<Node> &nodes,
                               int node_idx,
                               set<int> &processed_nodes)
{
    if (processed_nodes.find(node_idx) != processed_nodes.end()) return;
    processed_nodes.insert(node_idx);

    for (auto ait = nodes[node_idx].arcs.begin(); ait != nodes[node_idx].arcs.end(); ++ait) {
        nodes[ait->target_node].reverse_arcs.resize(nodes[ait->target_node].reverse_arcs.size()+1);
        nodes[ait->target_node].reverse_arcs.back().target_node = node_idx;
        set_reverse_arcs(nodes, ait->target_node, processed_nodes);
    }
}


void
DecoderGraph::clear_reverse_arcs(vector<Node> &nodes)
{
    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit)
        nit->reverse_arcs.clear();
}


void
DecoderGraph::tie_state_suffixes(vector<Node> &nodes,
                                 bool stop_propagation)
{
    set<int> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);
    tie_state_suffixes(nodes, processed_nodes, stop_propagation, END_NODE);
}


void
DecoderGraph::tie_state_suffixes(vector<Node> &nodes,
                                 set<int> &processed_nodes,
                                 bool stop_propagation,
                                 int node_idx)
{
    if (node_idx == START_NODE) return;
    if (processed_nodes.find(node_idx) != processed_nodes.end()) return;
    if (debug) cerr << endl << "tying state: " << node_idx << endl;
    //if (processed_nodes.size() % 1000 == 0) cerr << "number of processed nodes: "
    //                                             << processed_nodes.size() << "/" << nodes.size() << endl;
    processed_nodes.insert(node_idx);
    Node &nd = nodes[node_idx];

    map<pair<int, int>, set<int> > targets;
    for (auto ait = nd.reverse_arcs.begin(); ait != nd.reverse_arcs.end(); ++ait) {
        int target_hmm = nodes[ait->target_node].hmm_state;
        int word_id = nodes[ait->target_node].word_id;
        if (word_id != -1 && m_units[word_id].length() < 2) continue;
        //if (node_idx == END_NODE && target_hmm != -1) continue;
        if (nodes[ait->target_node].arcs.size() > 1) continue;
        targets[make_pair(word_id, target_hmm)].insert(ait->target_node);
    }

    if (debug) {
        int target_count = 0;
        for (auto tit = targets.begin(); tit !=targets.end(); ++tit)
            if (tit->second.size() > 1) target_count++;
        cerr << "total targets: " << target_count << endl;

        for (auto tit = targets.begin(); tit !=targets.end(); ++tit) {
            if (tit->second.size() == 1) continue;
            cerr << "target: " << tit->first.first << " " << tit->first.second << " " << tit->second.size() << endl;
            for (auto nit = tit->second.begin(); nit != tit->second.end(); ++nit)
                cerr << *nit << " ";
            cerr << endl;
        }
    }

    bool arcs_removed = false;
    for (auto tit = targets.begin(); tit !=targets.end(); ++tit) {
        if (tit->second.size() == 1) continue;

        auto nit = tit->second.begin();
        int tied_node_idx = *nit;
        nit++;
        while (nit != tit->second.end()) {
            int curr_node_idx = *nit;
            tied_node_idx = merge_nodes(nodes, tied_node_idx, curr_node_idx);
            arcs_removed = true;
            nit++;
        }

        if (debug) cerr << "\tnew tied node idx: " << tied_node_idx << endl;
    }

    if (stop_propagation && !arcs_removed) return;

    for (auto ait = nd.reverse_arcs.begin(); ait != nd.reverse_arcs.end(); ++ait)
        tie_state_suffixes(nodes, processed_nodes, stop_propagation, ait->target_node);
}


void
DecoderGraph::reachable_graph_nodes(vector<Node> &nodes,
                                    set<int> &node_idxs,
                                    int node_idx)
{
    node_idxs.insert(node_idx);
    for (auto ait = nodes[node_idx].arcs.begin(); ait != nodes[node_idx].arcs.end(); ++ait)
        if (node_idx != ait->target_node
            && node_idxs.find(ait->target_node) == node_idxs.end())
            reachable_graph_nodes(nodes, node_idxs, ait->target_node);
}


int
DecoderGraph::reachable_graph_nodes(vector<Node> &nodes)
{
    set<int> node_idxs;
    reachable_graph_nodes(nodes, node_idxs, START_NODE);
    return node_idxs.size();
}


void
DecoderGraph::prune_unreachable_nodes(vector<Node> &nodes)
{
    vector<Node> pruned_nodes;
    map<int,int> index_mapping;
    set<int> old_node_idxs;
    reachable_graph_nodes(nodes, old_node_idxs, START_NODE);
    int new_idx = 0;
    for (auto nit = old_node_idxs.begin(); nit != old_node_idxs.end(); ++nit) {
        index_mapping[*nit] = new_idx;
        new_idx++;
    }

    pruned_nodes.resize(old_node_idxs.size());
    for (auto nit = old_node_idxs.begin(); nit != old_node_idxs.end(); ++nit) {
        pruned_nodes[index_mapping[*nit]] = nodes[*nit];
        Node &new_node = pruned_nodes[index_mapping[*nit]];
        for (auto ait = new_node.arcs.begin(); ait != new_node.arcs.end(); ++ait) {
            ait->target_node = index_mapping[ait->target_node];
        }
    }

    nodes.swap(pruned_nodes);

    return;
}


void
DecoderGraph::add_hmm_self_transitions(std::vector<Node> &nodes)
{
    for (int i=0; i<nodes.size(); i++) {
        if (i == START_NODE) continue;
        if (i == END_NODE) continue;

        Node &node = nodes[i];
        if (node.hmm_state == -1) continue;

        HmmState &state = m_hmm_states[node.hmm_state];
        node.arcs.insert(node.arcs.begin(), Arc());
        node.arcs[0].log_prob = state.transitions[0].log_prob;
        node.arcs[0].target_node = i;
    }
}


void
DecoderGraph::set_hmm_transition_probs(std::vector<Node> &nodes)
{
    for (int i=0; i<nodes.size(); i++) {
        if (i == END_NODE) continue;

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
DecoderGraph::push_word_ids_left(vector<Node> &nodes,
                                 int &move_count,
                                 set<int> &processed_nodes,
                                 int node_idx,
                                 int prev_node_idx,
                                 int subword_id)
{
    if (node_idx == START_NODE) return;
    processed_nodes.insert(node_idx);

    Node &node = nodes[node_idx];

    if (debug && subword_id != -1) {
        cerr << "push left, node_idx: " << node_idx << " prev_node_idx: " << prev_node_idx;
        cerr << " subword to move: " << m_units[subword_id] << endl;
        cerr << endl;
    }

    if (subword_id != -1) {
        Node &prev_node = nodes[prev_node_idx];
        swap(node.word_id, prev_node.word_id);
        swap(node.hmm_state, prev_node.hmm_state);
        move_count++;
    }

    if (node.reverse_arcs.size() == 1) subword_id = node.word_id;
    else subword_id = -1;

    for (auto ait = node.reverse_arcs.begin(); ait != node.reverse_arcs.end(); ++ait) {
        if (ait->target_node == node_idx) throw string("Call push before setting self-transitions.");
        int temp_subword_id = subword_id;
        if (nodes[ait->target_node].arcs.size() > 1) temp_subword_id = -1;
        if (nodes[ait->target_node].word_id != -1) temp_subword_id = -1;
        if (processed_nodes.find(ait->target_node) == processed_nodes.end())
            push_word_ids_left(nodes, move_count, processed_nodes, ait->target_node, node_idx, temp_subword_id);
    }
}


void
DecoderGraph::push_word_ids_left(vector<Node> &nodes)
{
    set_reverse_arcs_also_from_unreachable(nodes);
    int move_count = 0;
    while (true) {
        set<int> processed_nodes;
        push_word_ids_left(nodes, move_count, processed_nodes);
        if (move_count == 0) break;
        move_count = 0;
    }
}


void
DecoderGraph::push_word_ids_right(vector<Node> &nodes,
                                  int &move_count,
                                  set<int> &processed_nodes,
                                  int node_idx,
                                  int prev_node_idx,
                                  int subword_id)
{
    if (node_idx == END_NODE) return;
    processed_nodes.insert(node_idx);

    Node &node = nodes[node_idx];

    if (debug && subword_id != -1) {
        cerr << "push right, node_idx: " << node_idx << " prev_node_idx: " << prev_node_idx;
        cerr << " subword to move: " << m_units[subword_id] << endl;
        cerr << endl;
    }

    if (subword_id != -1) {
        Node &prev_node = nodes[prev_node_idx];
        swap(node.word_id, prev_node.word_id);
        swap(node.hmm_state, prev_node.hmm_state);
        move_count++;
    }

    if (node.arcs.size() == 1) subword_id = node.word_id;
    else subword_id = -1;

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (ait->target_node == node_idx) throw string("Call push before setting self-transitions.");
        int temp_subword_id = subword_id;
        if (nodes[ait->target_node].reverse_arcs.size() > 1) temp_subword_id = -1;
        if (nodes[ait->target_node].word_id != -1) temp_subword_id = -1;
        if (processed_nodes.find(ait->target_node) == processed_nodes.end())
            push_word_ids_right(nodes, move_count, processed_nodes, ait->target_node, node_idx, temp_subword_id);
    }
}


void
DecoderGraph::push_word_ids_right(vector<Node> &nodes)
{
    set_reverse_arcs_also_from_unreachable(nodes);
    int move_count = 0;
    while (true) {
        set<int> processed_nodes;
        push_word_ids_right(nodes, move_count, processed_nodes);
        if (move_count == 0) break;
        move_count = 0;
    }
}


int
DecoderGraph::num_hmm_states(vector<Node> &nodes)
{
    set<int> node_idxs;
    reachable_graph_nodes(nodes, node_idxs, START_NODE);
    int hmm_state_count = 0;
    for (auto iit = node_idxs.begin(); iit != node_idxs.end(); ++iit)
        if (nodes[*iit].hmm_state != -1) hmm_state_count++;
    return hmm_state_count;
}


int
DecoderGraph::num_subword_states(vector<Node> &nodes)
{
    set<int> node_idxs;
    reachable_graph_nodes(nodes, node_idxs, START_NODE);
    int subword_state_count = 0;
    for (auto iit = node_idxs.begin(); iit != node_idxs.end(); ++iit)
        if (nodes[*iit].word_id != -1) subword_state_count++;
    return subword_state_count;
}


void
DecoderGraph::create_crossword_network(vector<Node> &nodes,
                                       map<string, int> &fanout,
                                       map<string, int> &fanin)
{
    for (auto wit = m_word_segs.begin(); wit != m_word_segs.end(); ++wit) {
        vector<string> triphones;
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit) {
            for (auto tit = m_lexicon[*swit].begin(); tit != m_lexicon[*swit].end(); ++tit) {
                triphones.push_back(*tit);
            }
        }
        if (triphones.size() < 2) {
            cerr << wit->first << endl;
            throw string("Warning, word " + wit->first + " with less than two phones");
        }
        string fanint = string("_-") + triphones[0][2] + string(1,'+') + triphones[1][2];
        string fanoutt = triphones[triphones.size()-2][2] + string(1,'-') + triphones[triphones.size()-1][2] + string("+_");
        fanout[fanoutt] = -1;
        fanin[fanint] = -1;
    }

    if (debug) {
        cerr << endl;
        cerr << "number of fan in nodes: " << fanin.size() << endl;
        for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit)
            cerr << "\t" << fiit->first << endl;
        cerr << "number of fan out nodes: " << fanout.size() << endl;
        for (auto foit = fanout.begin(); foit != fanout.end(); ++foit)
            cerr << "\t" << foit->first << endl;
    }

    map<string, int> connected_fanin_nodes;
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        nodes.resize(nodes.size()+1);
        foit->second = nodes.size()-1;

        for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
            string triphone1 = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + fiit->first[2];
            string triphone2 = foit->first[2] + string(1,'-') + fiit->first[2] + string(1,'+') + fiit->first[4];
            int idx = connect_triphone(nodes, triphone1, foit->second);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end()) {
                if (debug) cerr << "not found" << endl;
                idx = connect_triphone(nodes, triphone2, idx);
                connected_fanin_nodes[triphone2] = idx-2;
                if (fiit->second == -1) {
                    nodes.resize(nodes.size()+1);
                    fiit->second = nodes.size()-1;
                }
                nodes[idx].arcs.resize(nodes[idx].arcs.size()+1);
                nodes[idx].arcs.back().target_node = fiit->second;
                if (debug) cerr << "triphone1: " << triphone1 << " triphone2: " << triphone2
                                << " target node: " << fiit->second;
            }
            else {
                if (debug) cerr << "found" << endl;
                nodes[idx].arcs.resize(nodes[idx].arcs.size()+1);
                nodes[idx].arcs.back().target_node = connected_fanin_nodes[triphone2];
            }
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        cwnit->cw_node = true;
}


void
DecoderGraph::connect_crossword_network(vector<Node> &nodes,
                                        vector<Node> &cw_nodes,
                                        map<string, int> &fanout,
                                        map<string, int> &fanin)
{
    int offset = nodes.size();
    for (auto cwnit = cw_nodes.begin(); cwnit != cw_nodes.end(); ++cwnit) {
        nodes.push_back(*cwnit);
        for (auto ait = nodes.back().arcs.begin(); ait != nodes.back().arcs.end(); ++ait)
            ait->target_node += offset;
    }

    for (auto fonit = fanout.begin(); fonit != fanout.end(); ++fonit)
        fonit->second += offset;
    for (auto finit = fanin.begin(); finit != fanin.end(); ++finit)
        finit->second += offset;

    map<int, string> nodes_to_fanin;
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
        fanin_node.arcs.resize(fanin_node.arcs.size()+1);
        fanin_node.arcs.back().target_node = node_idx;
    }

    push_word_ids_left(nodes);

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
        node.arcs.resize(node.arcs.size()+1);
        node.arcs.back().target_node = fanout_idx;
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
        string &subword = m_units[node.word_id];
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

    for (auto ait = node.reverse_arcs.begin(); ait != node.reverse_arcs.end(); ++ait) {
        collect_cw_fanout_nodes(nodes, nodes_to_fanout, hmm_state_count,
                                phones, node_to_connect, ait->target_node);
    }
}


void
DecoderGraph::collect_cw_fanin_nodes(vector<Node> &nodes,
                                     map<int, string> &nodes_from_fanin,
                                     int hmm_state_count,
                                     vector<char> phones,
                                     int node_to_connect,
                                     int node_idx)
{
    if (node_idx == END_NODE) return;
    Node &node = nodes[node_idx];

    if (debug) cerr << endl << "in node: " << node_idx;

    if (node.hmm_state != -1) hmm_state_count++;
    if (node.word_id != -1) {
        string &subword = m_units[node.word_id];
        vector<string> &triphones = m_lexicon[subword];
        int tri_idx = 0;
        while (phones.size() < 2 && tri_idx < triphones.size()) {
            phones.push_back(triphones[tri_idx][2]);
            tri_idx++;
        }
    }

    if (node_to_connect == -1 &&
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
        if (debug) cerr << endl <<  "proceeding to node " << ait->target_node
                        << " from node: " << node_idx;
        collect_cw_fanin_nodes(nodes, nodes_from_fanin, hmm_state_count,
                               phones, node_to_connect, ait->target_node);
    }
}


void DecoderGraph::print_dot_digraph(vector<Node> &nodes, ostream &fstr)
{
    set<int> node_idxs;
    //reachable_graph_nodes(nodes, node_idxs);
    for (int i=0;i<nodes.size();i++)
        node_idxs.insert(i);

    fstr << "digraph {" << endl << endl;
    fstr << "\tnode [shape=ellipse,fontsize=30,fixedsize=false,width=0.95];" << endl;
    fstr << "\tedge [fontsize=12];" << endl;
    fstr << "\trankdir=LR;" << endl << endl;

    for (auto it=node_idxs.begin(); it != node_idxs.end(); ++it) {
        Node &nd = nodes[*it];
        fstr << "\t" << *it;
        if (*it == START_NODE) fstr << " [label=\"start\"]" << endl;
        else if (*it == END_NODE) fstr << " [label=\"end\"]" << endl;
        else if (nd.hmm_state != -1 && nd.word_id != -1)
            fstr << " [label=\"" << *it << ":" << nd.hmm_state << ", " << m_units[nd.word_id] << "\"]" << endl;
        else if (nd.hmm_state != -1 && nd.word_id == -1)
            fstr << " [label=\"" << *it << ":"<< nd.hmm_state << "\"]" << endl;
        else if (nd.hmm_state == -1 && nd.word_id != -1)
            fstr << " [label=\"" << *it << ":"<< m_units[nd.word_id] << "\"]" << endl;
        else
            fstr << " [label=\"" << *it << ":dummy\"]" << endl;
    }

    fstr << endl;
    for (auto nit=node_idxs.begin(); nit != node_idxs.end(); ++nit) {
        Node &node = nodes[*nit];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            fstr << "\t" << *nit << " -> " << ait->target_node << endl;
    }
    fstr << "}" << endl;
}


bool
DecoderGraph::nodes_identical(vector<Node> &nodes, int node_idx_1, int node_idx_2)
{
    Node &nd1 = nodes[node_idx_1];
    Node &nd2 = nodes[node_idx_2];
    if (nd1.word_id != nd2.word_id) return false;
    if (nd1.hmm_state != nd2.hmm_state) return false;
    if (nd1.arcs.size() != nd2.arcs.size()) return false;
    if (nd1.reverse_arcs.size() != nd2.reverse_arcs.size()) return false;

    set<pair<int, int> > node1_next_states;
    set<pair<int, int> > node2_next_states;
    for (auto ait = nd1.arcs.begin(); ait != nd1.arcs.end(); ++ait) {
        Node &target_node_1 = nodes[ait->target_node];
        node1_next_states.insert(make_pair(target_node_1.word_id, target_node_1.hmm_state));
    }
    for (auto ait = nd2.arcs.begin(); ait != nd2.arcs.end(); ++ait) {
        Node &target_node_2 = nodes[ait->target_node];
        node2_next_states.insert(make_pair(target_node_2.word_id, target_node_2.hmm_state));
    }
    if (node1_next_states != node2_next_states) return false;

    set<pair<int, int> > node1_reverse_next_states;
    set<pair<int, int> > node2_reverse_next_states;
    for (auto ait = nd1.reverse_arcs.begin(); ait != nd1.reverse_arcs.end(); ++ait) {
        Node &target_node_1 = nodes[ait->target_node];
        node1_reverse_next_states.insert(make_pair(target_node_1.word_id, target_node_1.hmm_state));
    }
    for (auto ait = nd2.reverse_arcs.begin(); ait != nd2.reverse_arcs.end(); ++ait) {
        Node &target_node_2 = nodes[ait->target_node];
        node2_reverse_next_states.insert(make_pair(target_node_2.word_id, target_node_2.hmm_state));
    }
    if (node1_reverse_next_states != node2_reverse_next_states) return false;

    return true;
}


int
DecoderGraph::merge_nodes(vector<Node> &nodes, int node_idx_1, int node_idx_2)
{
    if (node_idx_1 == node_idx_2) throw string("Merging same nodes.");

    Node &merged_node = nodes[node_idx_1];
    Node &removed_node = nodes[node_idx_2];
    removed_node.hmm_state = -1;
    removed_node.word_id = -1;

    set<int> arcs;
    for (auto ait = merged_node.arcs.begin(); ait != merged_node.arcs.end(); ++ait)
        arcs.insert(ait->target_node);
    for (auto ait = removed_node.arcs.begin(); ait != removed_node.arcs.end(); ++ait)
        arcs.insert(ait->target_node);

    set<int> reverse_arcs;
    for (auto ait = merged_node.reverse_arcs.begin(); ait != merged_node.reverse_arcs.end(); ++ait)
        reverse_arcs.insert(ait->target_node);
    for (auto ait = removed_node.reverse_arcs.begin(); ait != removed_node.reverse_arcs.end(); ++ait)
        reverse_arcs.insert(ait->target_node);

    merged_node.arcs.clear(); merged_node.reverse_arcs.clear();
    removed_node.arcs.clear(); removed_node.reverse_arcs.clear();

    for (auto ait=arcs.begin(); ait != arcs.end(); ++ait) {
        merged_node.arcs.resize(merged_node.arcs.size()+1);
        merged_node.arcs.back().target_node = *ait;
        Node &target_node = nodes[*ait];
        for (auto rait = target_node.reverse_arcs.begin(); rait != target_node.reverse_arcs.end();) {
            if (rait->target_node == node_idx_1 || rait->target_node == node_idx_2)
                rait = target_node.reverse_arcs.erase(rait);
            else ++rait;
        }
        target_node.reverse_arcs.resize(target_node.reverse_arcs.size()+1);
        target_node.reverse_arcs.back().target_node = node_idx_1;
    }

    for (auto ait=reverse_arcs.begin(); ait != reverse_arcs.end(); ++ait) {
        merged_node.reverse_arcs.resize(merged_node.reverse_arcs.size()+1);
        merged_node.reverse_arcs.back().target_node = *ait;
        Node &target_node = nodes[*ait];
        for (auto rait = target_node.arcs.begin(); rait != target_node.arcs.end();) {
            if (rait->target_node == node_idx_1 || rait->target_node == node_idx_2)
                rait = target_node.arcs.erase(rait);
            else ++rait;
        }
        target_node.arcs.resize(target_node.arcs.size()+1);
        target_node.arcs.back().target_node = node_idx_1;
    }

    return node_idx_1;
}


void
DecoderGraph::connect_end_to_start_node(vector<Node> &nodes)
{
    nodes[END_NODE].arcs.resize(nodes[1].arcs.size()+1);
    nodes[END_NODE].arcs.back().target_node = START_NODE;
}


void
DecoderGraph::write_graph(vector<Node> &nodes, string fname)
{
    std::ofstream outf(fname);
    outf << nodes.size() << endl;;
    for (int i=0; i<nodes.size(); i++)
        outf << "n " << i << " " << nodes[i].hmm_state << " " << nodes[i].word_id << " " << nodes[i].arcs.size() << endl;
    for (int i=0; i<nodes.size(); i++) {
        Node &node = nodes[i];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            outf << "a " << i << " " << ait->target_node << endl;
    }
}
