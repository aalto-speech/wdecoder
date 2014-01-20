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
DecoderGraph::print_word_graph(vector<SubwordNode> &nodes,
                               vector<int> path,
                               int node_idx)
{
    path.push_back(node_idx);

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
                                   int debug,
                                   int sw_node_idx,
                                   int node_idx,
                                   char left_context,
                                   char second_left_context)
{
    if (sw_node_idx == END_NODE) return;

    const SubwordNode &swnode = swnodes[sw_node_idx];
    if (swnode.subword_id == -1) {
        for (auto ait = swnode.out_arcs.begin(); ait != swnode.out_arcs.end(); ++ait)
            if (ait->second != END_NODE)
                expand_subword_nodes(swnodes, nodes, debug, ait->second, node_idx, left_context, second_left_context);
            // FIXME: need to handle end node case ?
        return;
    }

    string subword = m_units[swnode.subword_id];
    if (debug) cerr << subword << "\tleft context: " << left_context << endl;
    auto triphones = m_lexicon[subword];

    // Construct the left connecting triphone and expand states
    if (second_left_context != '_' && left_context != '_') {
        string triphone = string(1,second_left_context) + "-" + string(1,left_context) + "+" + string(1,triphones[0][2]);
        node_idx = connect_triphone(nodes, triphone, node_idx, debug);
    }

    for (int tidx = 0; tidx < triphones.size()-1; ++tidx) {
        string triphone = triphones[tidx];
        if (tidx == 0) triphone = left_context + triphone.substr(1);
        node_idx = connect_triphone(nodes, triphone, node_idx, debug);
    }
    if (debug) cerr << endl;

    // Connect dummy node with subword id
    if (debug) cerr << "adding dummy node for subword: " << m_units[swnode.subword_id]
                    << " subword node idx: " << sw_node_idx << endl;
    nodes.resize(nodes.size()+1);
    nodes.back().word_id = swnode.subword_id;
    nodes[node_idx].arcs.resize(nodes[node_idx].arcs.size()+1);
    nodes[node_idx].arcs.back().target_node = nodes.size()-1;
    node_idx = nodes.size()-1;

    char last_phone = triphones[triphones.size()-1][2];
    char second_last_phone = left_context;
    if (triphones.size() > 1) second_last_phone = triphones[triphones.size()-2][2];
    for (auto ait = swnode.out_arcs.begin(); ait != swnode.out_arcs.end(); ++ait)
        if (ait->second != END_NODE)
            expand_subword_nodes(swnodes, nodes, debug, ait->second, node_idx, last_phone, second_last_phone);
        else {
            string triphone = string(1,second_last_phone) + "-" + string(1,last_phone) + "+_";
            int temp_node_idx = connect_triphone(nodes, triphone, node_idx, debug);
            nodes[temp_node_idx].arcs.resize(nodes[temp_node_idx].arcs.size()+1);
            nodes[temp_node_idx].arcs.back().target_node = END_NODE;
        }
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
                               int node_idx,
                               int debug)
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
        vector<int> words;
        cout << "hmm states:";
        for (int i=0; i<path.size(); i++) {
            if (nodes[path[i]].hmm_state != -1)
                cout << " " << nodes[path[i]].hmm_state;
            if (nodes[path[i]].word_id != -1)
                words.push_back(nodes[path[i]].word_id);
        }
        cout << endl;
        if (words.size()) {
            cout << "subwords:";
            for (auto it=words.begin(); it != words.end(); ++it)
                cout << " " << m_units[*it];
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
                                 int debug,
                                 int node_idx)
{
    if (debug) cerr << endl << "tying state: " << node_idx << endl;
    if (node_idx == END_NODE) return;
    Node &nd = nodes[node_idx];

    map<int, set<int> > targets;
    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait) {
        int target_hmm = nodes[ait->target_node].hmm_state;
        if (target_hmm != -1) targets[target_hmm].insert(ait->target_node);
    }

    if (debug) {
        cerr << "total targets: " << targets.size() << endl;
        for (auto tit = targets.begin(); tit !=targets.end(); ++tit) {
            cerr << tit->first << " " << tit->second.size() << endl;
            for (auto nit = tit->second.begin(); nit != tit->second.end(); ++nit)
                cerr << *nit << " ";
            cerr << endl;
        }
    }

    set<int> arcs_to_remove;
    for (auto tit = targets.begin(); tit !=targets.end(); ++tit) {
        if (tit->second.size() == 1) continue;
        int tied_node_idx = *(tit->second.begin());
        for (auto nit = tit->second.rbegin(); nit != tit->second.rend(); ++nit) {
            int curr_node_idx = *nit;
            if (tied_node_idx == curr_node_idx) break;
            Node &temp_nd = nodes[curr_node_idx];
            for (auto ait = temp_nd.arcs.begin(); ait != temp_nd.arcs.end(); ++ait)
                nodes[tied_node_idx].arcs.push_back(*ait);
            arcs_to_remove.insert(curr_node_idx);
        }
    }

    if (debug) cerr << "arcs to remove: " << arcs_to_remove.size() << endl;
    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end();) {
        if (arcs_to_remove.find(ait->target_node) != arcs_to_remove.end()) {
            if (debug) cerr << "erasing arc to: " << ait->target_node << endl;
            ait = nd.arcs.erase(ait);
        }
        else ++ait;
    }

    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait)
        tie_state_prefixes(nodes, debug, ait->target_node);
}


void
DecoderGraph::reachable_graph_nodes(vector<Node> &nodes,
                                    set<int> &node_idxs,
                                    int node_idx)
{
    node_idxs.insert(node_idx);
    for (auto ait = nodes[node_idx].arcs.begin(); ait != nodes[node_idx].arcs.end(); ++ait)
        if (node_idx != ait->target_node)
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


// FIXME: refactor this to connect_triphone
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
DecoderGraph::push_word_ids_left(std::vector<Node> &nodes,
                                 bool debug,
                                 int node_idx,
                                 int prev_node_idx,
                                 int first_hmm_node_wo_branching)
{
    if (node_idx == END_NODE) return;

    Node &node = nodes[node_idx];

    if (node.word_id != -1) {
        if (first_hmm_node_wo_branching != -1) {
            if (debug) cerr << "Pushing subword " << m_units[node.word_id]
                            << " from node " << node_idx << " to " << first_hmm_node_wo_branching << endl;
            // Move the out transitions of the subword node to the last HMM node
            nodes[prev_node_idx].arcs = node.arcs;
            // Create a new identical HMM node for the first state
            nodes.push_back(nodes[first_hmm_node_wo_branching]);
            nodes[first_hmm_node_wo_branching].arcs[0].target_node = nodes.size()-1;
            nodes[first_hmm_node_wo_branching].word_id = node.word_id;
            nodes[first_hmm_node_wo_branching].hmm_state = -1;
            first_hmm_node_wo_branching = -1;
        }
    }

    if (node.arcs.size() > 1)
        first_hmm_node_wo_branching = -1;
    else
        if (first_hmm_node_wo_branching == -1) first_hmm_node_wo_branching = node_idx;

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (ait->target_node == node_idx) throw string("Call push before setting self-transitions.");
        push_word_ids_left(nodes, debug, ait->target_node, node_idx, first_hmm_node_wo_branching);
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
