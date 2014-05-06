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
#include "SubwordGraphBuilder.hh"
#include "gutils.hh"

using namespace std;
using namespace gutils;


namespace SubwordGraphBuilder {


void
create_crossword_network(vector<DecoderGraph::Node> &nodes,
                         map<string, int> &fanout,
                         map<string, int> &fanin)
{
    set<string> one_phone_subwords;
    for (auto swit = m_lexicon.begin(); swit != m_lexicon.end(); ++swit) {
        vector<string> &triphones = swit->second;
        if (triphones.size() == 0) continue;
        else if (triphones.size() == 1) {
            one_phone_subwords.insert(swit->first);
            string fanint = triphones[0];
            string fanoutt = triphones[0];
            fanout[fanoutt] = -1;
            fanin[fanint] = -1;
        }
        else {
            string fanint = string("_-") + triphones[0][2] + string(1,'+') + triphones[1][2];
            string fanoutt = triphones[triphones.size()-2][2] + string(1,'-') + triphones[triphones.size()-1][2] + string("+_");
            fanout[fanoutt] = -1;
            fanin[fanint] = -1;
        }
    }

    map<string, int> connected_fanin_nodes;
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        nodes.resize(nodes.size()+1);
        nodes.back().flags |= NODE_FAN_OUT_DUMMY;
        foit->second = nodes.size()-1;

        for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
            string triphone1 = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + fiit->first[2];
            string triphone2 = foit->first[2] + string(1,'-') + fiit->first[2] + string(1,'+') + fiit->first[4];
            int idx = connect_triphone(nodes, triphone1, foit->second);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end()) {
                idx = connect_triphone(nodes, triphone2, idx);
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

    // Add loops for one phone subwords to cross-word network
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {
        for (auto opswit = one_phone_subwords.begin(); opswit != one_phone_subwords.end(); ++opswit) {
            string single_phone = m_lexicon[*opswit][0];
            string triphone = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + single_phone[2];
            int idx = connect_triphone(nodes, triphone, foit->second);
            idx = connect_word(nodes, *opswit, idx);
            string fanout_loop_connector = foit->first[2] + string(1,'-') + single_phone[2] + string("+_");
            nodes[idx].arcs.insert(fanout[fanout_loop_connector]);
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        if (cwnit->flags == 0) cwnit->flags |= NODE_CW;
}


void
connect_crossword_network(vector<DecoderGraph::Node> &nodes,
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
    collect_cw_fanin_nodes(nodes, nodes_to_fanin);

    for (auto finit = nodes_to_fanin.begin(); finit != nodes_to_fanin.end(); ++finit) {
        if (fanin.find(finit->second) == fanin.end()) {
            cerr << "Problem, triphone: " << finit->second << " not found in fanin" << endl;
            assert(false);
        }
        int fanin_idx = fanin[finit->second];
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
        Node &node = nodes[fonit->first];
        node.arcs.insert(fanout_idx);
    }
}


void
connect_one_phone_subwords_from_start_to_cw(vector<DecoderGraph::Node> &nodes,
                                            map<string, int> &fanout)
{
    for (auto swit = m_lexicon.begin(); swit != m_lexicon.end(); ++swit) {
        vector<string> &triphones = swit->second;
        if (triphones.size() != 1 || triphones[0].length() == 1) continue;
        string fanoutt = triphones[0];
        //cerr << "connecting word " << swit->first << " from start to cw" << endl;
        int idx = connect_word(nodes, swit->first, START_NODE);
        nodes[idx].arcs.insert(fanout[fanoutt]);
    }
}


void
connect_one_phone_subwords_from_cw_to_end(vector<DecoderGraph::Node> &nodes,
                                          map<string, int> &fanin)
{
    for (auto swit = m_lexicon.begin(); swit != m_lexicon.end(); ++swit) {
        vector<string> &triphones = swit->second;
        if (triphones.size() != 1 || triphones[0].length() == 1) continue;
        string fanint = triphones[0];
        //cerr << "connecting word " << swit->first << " from cw to end" << endl;
        int idx = connect_word(nodes, swit->first, fanin[fanint]);
        nodes[idx].arcs.insert(END_NODE);
    }
}


};

