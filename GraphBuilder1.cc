#include <cassert>
#include <sstream>

#include "GraphBuilder1.hh"
#include "gutils.hh"

using namespace std;
using namespace gutils;


namespace graphbuilder1 {


void
create_word_graph(DecoderGraph &dg,
        vector<SubwordNode> &nodes,
        vector<pair<string, vector<string> > > &word_segs)
{
    nodes.resize(2);

    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit) {

        vector<vector<int> > word_sw_nodes;
        bool tie_first = false;
        for (unsigned int swidx = 0; swidx < wit->second.size(); ++swidx) {

            string curr_subword = (wit->second)[swidx];
            if (swidx == 0 && dg.m_lexicon[curr_subword].size() == 1) {
                tie_first = true;
            }
            else if (swidx == 1 && tie_first) {
                vector<int> tmp;
                int subword_idx = dg.m_subword_map[(wit->second)[0]];
                tmp.push_back(subword_idx);
                subword_idx = dg.m_subword_map[curr_subword];
                tmp.push_back(subword_idx);
                word_sw_nodes.push_back(tmp);
            }
            else if (swidx == (wit->second.size()-1) && dg.m_lexicon[curr_subword].size() == 1) {
                int curr_subword_idx = dg.m_subword_map[curr_subword];
                word_sw_nodes.back().push_back(curr_subword_idx);
            }
            else {
                vector<int> tmp;
                int subword_idx = dg.m_subword_map[curr_subword];
                tmp.push_back(subword_idx);
                word_sw_nodes.push_back(tmp);
            }
        }

        int curr_nd = START_NODE;
        for (unsigned int swnidx = 0; swnidx < word_sw_nodes.size(); ++swnidx) {
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
tie_subword_suffixes(vector<SubwordNode> &nodes)
{
    map<vector<int>, int> suffix_counts;
    SubwordNode &node = nodes[END_NODE];
    vector<int> empty;

    for (auto sit = node.in_arcs.begin(); sit != node.in_arcs.end(); ++sit) {
        if (nodes[sit->second].in_arcs[0].second == START_NODE
            || nodes[sit->second].out_arcs.size() > 1) continue;
        suffix_counts[sit->first] += 1;
    }

    for (auto sit = suffix_counts.begin(); sit != suffix_counts.end(); ++sit) {
        if (sit->second > 1) {
            nodes.resize(nodes.size()+1);
            nodes.back().subword_ids = sit->first;

            nodes.back().out_arcs[empty] = END_NODE;
            for (auto ait = node.in_arcs.begin(); ait != node.in_arcs.end(); ++ait) {
                // Tie only real suffixes ie. not connected from start node
                if (ait->first == sit->first && (nodes[ait->second].in_arcs[0].second != START_NODE)
                    && nodes[ait->second].out_arcs.size() == 1) {
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
print_word_graph(DecoderGraph &dg,
                vector<SubwordNode> &nodes,
                               vector<int> path,
                               int node_idx)
{
    path.push_back(node_idx);

    for (auto ait = nodes[node_idx].out_arcs.begin(); ait != nodes[node_idx].out_arcs.end(); ++ait) {
        if (ait->second == END_NODE) {
            for (unsigned int i=0; i<path.size(); i++) {
                vector<int> subword_ids = nodes[path[i]].subword_ids;
                if (subword_ids.size() == 0) continue;
                for (unsigned int swi=0; swi<subword_ids.size(); ++swi) {
                    if (swi>0) cout << " ";
                    if (subword_ids[swi] >= 0) cout << dg.m_subwords[subword_ids[swi]];
                }
                if (path[i] >= 0) cout << " (" << path[i] << ")";
                if (i+1 != path.size()) cout << " ";
            }
            cout << endl;
        }
        else {
            print_word_graph(dg, nodes, path, ait->second);
        }
    }
}


void
print_word_graph(DecoderGraph &dg,
            vector<SubwordNode> &nodes)
{
    vector<int> path;
    print_word_graph(dg, nodes, path, START_NODE);
}


void
reachable_word_graph_nodes(vector<SubwordNode> &nodes,
                                         set<int> &node_idxs,
                                         int node_idx)
{
    node_idxs.insert(node_idx);
    for (auto ait = nodes[node_idx].out_arcs.begin(); ait != nodes[node_idx].out_arcs.end(); ++ait)
        reachable_word_graph_nodes(nodes, node_idxs, ait->second);
}


int
reachable_word_graph_nodes(vector<SubwordNode> &nodes)
{
    set<int> node_idxs;
    reachable_word_graph_nodes(nodes, node_idxs, START_NODE);
    return node_idxs.size();
}


void
triphonize_subword_nodes(DecoderGraph &dg,
                    vector<SubwordNode> &swnodes)
{
    for (auto nit = swnodes.begin(); nit != swnodes.end(); ++nit) {

        if (nit->subword_ids.size() == 0) continue;

        string tripstring;
        for (auto swit = nit->subword_ids.begin(); swit != nit->subword_ids.end(); ++swit) {
            string sw = dg.m_subwords[*swit];
            vector<string> &trips = dg.m_lexicon[sw];
            for (auto tit = trips.begin(); tit != trips.end(); ++tit)
                tripstring += (*tit)[2];
        }

        gutils::triphonize(tripstring, nit->triphones);
    }
}


void
expand_subword_nodes(DecoderGraph &dg,
                vector<SubwordNode> &swnodes,
                                   vector<DecoderGraph::Node> &nodes)
{
    nodes.resize(2);
    triphonize_subword_nodes(dg, swnodes);
    map<unsigned int, unsigned int> expanded_nodes;
    expand_subword_nodes(dg, swnodes, nodes, expanded_nodes);
}


void
expand_subword_nodes(DecoderGraph &dg,
            const vector<SubwordNode> &swnodes,
                                   vector<DecoderGraph::Node> &nodes,
                                   map<sw_node_idx_t, node_idx_t> &expanded_nodes,
                                   sw_node_idx_t sw_node_idx,
                                   node_idx_t node_idx,
                                   char left_context,
                                   char second_left_context)
{
    if (sw_node_idx == END_NODE) return;

    const SubwordNode &swnode = swnodes[sw_node_idx];

    if (sw_node_idx == START_NODE) {
        for (auto ait = swnode.out_arcs.begin(); ait != swnode.out_arcs.end(); ++ait)
            if (ait->second != END_NODE)
                expand_subword_nodes(dg, swnodes, nodes, expanded_nodes, ait->second,
                                     node_idx, left_context, second_left_context);
        return;
    }

    vector<string> triphones = swnode.triphones;

    // Construct the first left connecting triphone
    if (second_left_context != '_' && left_context != '_') {
        string triphone = string(1,second_left_context) + "-" + string(1,left_context) + "+" + string(1,triphones[0][2]);
        node_idx = connect_triphone(dg, nodes, triphone, node_idx);
    }

    // Construct the second left connecting triphone if possible
    if (triphones.size() > 1) {
        string triphone = string(1,left_context) + triphones[0].substr(1);
        node_idx = connect_triphone(dg, nodes, triphone, node_idx);
    }

    // Body triphones
    // Use previously expanded states if possible
    auto previous = expanded_nodes.find(sw_node_idx);
    if (previous == expanded_nodes.end()) {
        for (unsigned int tidx = 1; tidx < triphones.size()-1; ++tidx) {
            string triphone = triphones[tidx];
            node_idx = connect_triphone(dg, nodes, triphone, node_idx);
            if (tidx == 1) expanded_nodes[sw_node_idx] = node_idx-2;
        }
    }
    else {
        nodes[node_idx].arcs.insert(previous->second);
        return;
    }

    char last_phone = triphones[triphones.size()-1][2];
    char second_last_phone = left_context;
    if (triphones.size() > 1) second_last_phone = triphones[triphones.size()-2][2];

    if (swnode.out_arcs.size() == 1 && swnode.out_arcs.begin()->second == END_NODE) {
        string triphone = string(1,second_last_phone) + "-" + string(1,last_phone) + "+_";
        int temp_node_idx = connect_triphone(dg, nodes, triphone, node_idx);

        for (auto swit = swnode.subword_ids.begin(); swit != swnode.subword_ids.end(); ++swit) {
            nodes.resize(nodes.size()+1);
            nodes.back().word_id = *swit;
            nodes[temp_node_idx].arcs.insert(nodes.size()-1);
            temp_node_idx = nodes.size()-1;
        }

        nodes[temp_node_idx].arcs.insert(END_NODE);
        return;
    }

    for (auto swit = swnode.subword_ids.begin(); swit != swnode.subword_ids.end(); ++swit) {
        nodes.resize(nodes.size()+1);
        nodes.back().word_id = *swit;
        nodes[node_idx].arcs.insert(nodes.size()-1);
        node_idx = nodes.size()-1;
    }

    for (auto ait = swnode.out_arcs.begin(); ait != swnode.out_arcs.end(); ++ait) {
        if (ait->second == END_NODE) {
            string triphone = string(1,second_last_phone) + "-" + string(1,last_phone) + "+_";
            int temp_node_idx = connect_triphone(dg, nodes, triphone, node_idx);
            nodes[temp_node_idx].arcs.insert(END_NODE);
        }
        else
            expand_subword_nodes(dg, swnodes, nodes, expanded_nodes, ait->second,
                                 node_idx, last_phone, second_last_phone);
    }
}


void
create_crossword_network(DecoderGraph &dg,
                        std::vector<std::pair<std::string, std::vector<std::string> > > &word_segs,
                        vector<DecoderGraph::Node> &nodes,
                                       map<string, int> &fanout,
                                       map<string, int> &fanin)
{
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit) {
        vector<string> triphones;
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit) {
            for (auto tit = dg.m_lexicon[*swit].begin(); tit != dg.m_lexicon[*swit].end(); ++tit) {
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

    map<string, int> connected_fanin_nodes;
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        nodes.resize(nodes.size()+1);
        nodes.back().flags |= NODE_FAN_OUT_DUMMY;
        foit->second = nodes.size()-1;

        for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
            string triphone1 = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + fiit->first[2];
            string triphone2 = foit->first[2] + string(1,'-') + fiit->first[2] + string(1,'+') + fiit->first[4];
            int idx = connect_triphone(dg, nodes, triphone1, foit->second);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end()) {
                idx = connect_triphone(dg, nodes, triphone2, idx);
                connected_fanin_nodes[triphone2] = idx-2;
                if (fiit->second == -1) {
                    nodes.resize(nodes.size()+1);
                    nodes.back().flags |= NODE_FAN_IN_DUMMY;
                    fiit->second = nodes.size()-1;
                }
                nodes[idx].arcs.insert(fiit->second);
            }
            else
                nodes[idx].arcs.insert(connected_fanin_nodes[triphone2]);
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        if (cwnit->flags == 0) cwnit->flags |= NODE_CW;
}



void
connect_crossword_network(DecoderGraph &dg,
                        vector<DecoderGraph::Node> &nodes,
                                   vector<DecoderGraph::Node> &cw_nodes,
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
    collect_cw_fanin_nodes(dg, nodes, nodes_to_fanin);

    for (auto finit = nodes_to_fanin.begin(); finit != nodes_to_fanin.end(); ++finit) {
        if (fanin.find(finit->second) == fanin.end()) {
            cerr << "Problem, triphone: " << finit->second << " not found in fanin" << endl;
            assert(false);
        }
        int fanin_idx = fanin[finit->second];
        DecoderGraph::Node &fanin_node = nodes[fanin_idx];
        int node_idx = finit->first;
        fanin_node.arcs.insert(node_idx);
    }

    if (push_left_after_fanin)
        push_word_ids_left(nodes);
    else
        set_reverse_arcs_also_from_unreachable(nodes);

    map<int, string> nodes_to_fanout;

    for (auto fonit = nodes_to_fanout.begin(); fonit != nodes_to_fanout.end(); ++fonit) {
        if (fanout.find(fonit->second) == fanout.end()) {
            cerr << "Problem, triphone: " << fonit->second << " not found in fanout" << endl;
            assert(false);
        }
        int fanout_idx = fanout[fonit->second];
        DecoderGraph::Node &node = nodes[fonit->first];
        node.arcs.insert(fanout_idx);
    }
}


}

