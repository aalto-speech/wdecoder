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


namespace subwordgraphbuilder {


void
create_crossword_network(DecoderGraph &dg,
                         set<string> &subwords,
                         vector<DecoderGraph::Node> &nodes,
                         map<string, int> &fanout,
                         map<string, int> &fanin)
{
    set<string> one_phone_subwords;
    set<char> phones;
    for (auto swit = dg.m_lexicon.begin(); swit != dg.m_lexicon.end(); ++swit) {
        if (subwords.find(swit->first) == subwords.end()) continue;
        vector<string> &triphones = swit->second;
        if (triphones.size() == 0) continue;
        else if (triphones.size() == 1 && triphones[0].length() == 5) {
            one_phone_subwords.insert(swit->first);
            phones.insert(triphones[0][2]);
            fanout[triphones[0]] = -1;
            fanin[triphones[0]] = -1;
        }
        else if (triphones.size() > 1) {
            fanout[triphones.back()] = -1;
            fanin[triphones[0]] = -1;
        }
    }

    // All phone-phone combinations from one phone subwords to both fanout and fanin
    for (auto fphit = phones.begin(); fphit != phones.end(); ++fphit) {
        for (auto sphit = phones.begin(); sphit != phones.end(); ++sphit) {
            string fanint = string("_-") + string(1,*fphit) + string(1,'+') + string(1,*sphit);
            string fanoutt = string(1,*fphit) + string(1,'-') + string(1,*sphit) + string("+_");
            fanout[fanoutt] = -1;
            fanin[fanint] = -1;
        }
    }

    // Fanin last triphone + phone from one phone subwords, all combinations to fanin
    for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
        if ((fiit->first)[4] == '_') continue;
        for (auto phit = phones.begin(); phit != phones.end(); ++phit) {
            string fanint = string("_-") + string(1,(fiit->first)[4]) + string(1,'+') + string(1,*phit);
            fanin[fanint] = -1;
        }
    }

    // Fanout last triphone + phone from one phone subwords, all combinations to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {
        if ((foit->first)[0] == '_') continue;
        for (auto phit = phones.begin(); phit != phones.end(); ++phit) {
            string fanoutt = string(1,(foit->first)[2]) + string(1,'-') + string(1,*phit) + string("+_");
            fanout[fanoutt] = -1;
        }
    }

    map<string, int> connected_fanin_nodes;
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        nodes.resize(nodes.size()+1);
        nodes.back().flags |= NODE_FAN_OUT_DUMMY;
        foit->second = nodes.size()-1;
        //nodes[foit->second].label.assign(foit->first);
        int start_index = foit->second;

        for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
            string triphone1 = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + fiit->first[2];
            string triphone2 = foit->first[2] + string(1,'-') + fiit->first[2] + string(1,'+') + fiit->first[4];

            int tri1_idx = connect_triphone(dg, nodes, triphone1, start_index);
            int idx = connect_word(dg, nodes, "<w>", tri1_idx);
            idx = connect_triphone(dg, nodes, "_", idx);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end())
            {
                idx = connect_triphone(dg, nodes, triphone2, idx);
                nodes[tri1_idx].arcs.insert(idx-2);
                connected_fanin_nodes[triphone2] = idx-2;
                if (fiit->second == -1) {
                    nodes.resize(nodes.size()+1);
                    nodes.back().flags |= NODE_FAN_IN_DUMMY;
                    fiit->second = nodes.size()-1;
                    //nodes[fiit->second].label.assign(fiit->first);
                }
                nodes[idx].arcs.insert(fiit->second);
            }
            else {
                nodes[tri1_idx].arcs.insert(connected_fanin_nodes[triphone2]);
                nodes[idx].arcs.insert(connected_fanin_nodes[triphone2]);
            }
        }
    }


    // Add loops for one phone subwords from fanout back to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        // Connect one phone subwords
        // Both to xy_ and _x_ fanin connectors
        // Optional short silence and word break
        for (auto opswit = one_phone_subwords.begin(); opswit != one_phone_subwords.end(); ++opswit) {
            string single_phone = dg.m_lexicon[*opswit][0];
            string triphone = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + string(1,single_phone[2]);

            int tridx = connect_triphone(dg, nodes, triphone, foit->second);

            // just subword
            int lidx = connect_word(dg, nodes, *opswit, tridx);
            // optionally word boundary after subword
            int wbidx = connect_word(dg, nodes, "<w>", lidx);
            wbidx = connect_triphone(dg, nodes, "_", wbidx);

            // word boundary first then subword
            int l2idx = connect_word(dg, nodes, "<w>", tridx);
            l2idx = connect_word(dg, nodes, *opswit, l2idx);
            l2idx = connect_triphone(dg, nodes, "_", l2idx);

            string fanout_loop_connector = foit->first[2] + string(1,'-') + string(1,single_phone[2]) + string("+_");
            if (fanout.find(fanout_loop_connector) == fanout.end()) {
                cerr << "problem in connecting fanout loop for one phone subword:" << *opswit << endl;
                cerr << fanout_loop_connector << endl;
                assert(false);
            }
            nodes[lidx].arcs.insert(fanout[fanout_loop_connector]);
            nodes[wbidx].arcs.insert(fanout[fanout_loop_connector]);
            nodes[l2idx].arcs.insert(fanout[fanout_loop_connector]);
        }
    }


    // Add loops for one phone subwords from fanin back to fanin
    for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
        if (fiit->first[0] == '_' && fiit->first[4] == '_') continue;

        // Connect one phone subwords
        // Both to _xy and _x_ fanin connectors
        // Optional short silence and word break
        for (auto opswit = one_phone_subwords.begin(); opswit != one_phone_subwords.end(); ++opswit) {
            string single_phone = dg.m_lexicon[*opswit][0];
            string triphone = fiit->first[2] + string(1,'-') + fiit->first[4] + string(1,'+') + string(1,single_phone[2]);

            int lidx = connect_word(dg, nodes, *opswit, fiit->second);
            int wbidx = connect_word(dg, nodes, "<w>", lidx);
            wbidx = connect_triphone(dg, nodes, "_", wbidx);
            lidx = connect_triphone(dg, nodes, triphone, lidx);
            nodes[wbidx].arcs.insert(lidx-2);
            string fanin_loop_connector = string("_-") + fiit->first[4] + string(1,'+') + string(1,single_phone[2]);
            if (fanin.find(fanin_loop_connector) == fanin.end()) {
                cerr << "problem in connecting fanin loop for one phone subword:" << *opswit << endl;
                cerr << fanin_loop_connector << endl;
                assert(false);
            }
            nodes[lidx].arcs.insert(fanin[fanin_loop_connector]);

            string triphone2 = fiit->first[4] + string(1,'-') + string(1,single_phone[2]) + string("+_");

            lidx = connect_word(dg, nodes, *opswit, fiit->second);
            wbidx = connect_word(dg, nodes, "<w>", lidx);
            wbidx = connect_triphone(dg, nodes, "_", wbidx);
            lidx = connect_triphone(dg, nodes, triphone2, lidx);
            nodes[wbidx].arcs.insert(lidx-2);
            fanin_loop_connector = string("_-") + string(1,single_phone[2]) + string("+_");
            if (fanin.find(fanin_loop_connector) == fanin.end()) {
                cerr << "problem in connecting fanin loop for one phone subword:" << *opswit << endl;
                assert(false);
            }
            nodes[lidx].arcs.insert(fanin[fanin_loop_connector]);
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
    collect_cw_fanout_nodes(dg, nodes, nodes_to_fanout);

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


void
connect_one_phone_subwords_from_start_to_cw(DecoderGraph &dg,
                                            set<string> &subwords,
                                            vector<DecoderGraph::Node> &nodes,
                                            map<string, int> &fanout)
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        vector<string> &triphones = dg.m_lexicon[*swit];
        if (triphones.size() != 1 || triphones[0].length() == 1) continue;
        string fanoutt = triphones[0];
        int idx = connect_word(dg, nodes, *swit, START_NODE);
        if (fanout.find(fanoutt) == fanout.end()) {
            cerr << "problem in connecting: " << *swit << " from start to fanout" << endl;
            assert(false);
        }
        nodes[idx].arcs.insert(fanout[fanoutt]);
    }
}


void
connect_one_phone_subwords_from_cw_to_end(DecoderGraph &dg,
                                          set<string> &subwords,
                                          vector<DecoderGraph::Node> &nodes,
                                          map<string, int> &fanin)
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        vector<string> &triphones = dg.m_lexicon[*swit];
        if (triphones.size() != 1 || triphones[0].length() == 1) continue;
        string fanint = triphones[0];
        if (fanin.find(fanint) == fanin.end()) {
            cerr << "problem in connecting: " << *swit << " fanin to end" << endl;
            assert(false);
        }
        int idx = connect_word(dg, nodes, *swit, fanin[fanint]);
        nodes[idx].arcs.insert(END_NODE);
    }
}


}

