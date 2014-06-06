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
    cerr << endl << "step 0" << endl;

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
            cerr << "fanint: " << triphones[0] << endl;
            cerr << "fanoutt: " << triphones[0] << endl;
        }
        else if (triphones.size() > 1) {
            fanout[triphones.back()] = -1;
            fanin[triphones[0]] = -1;
            cerr << "fanint: " << triphones[0] << endl;
            cerr << "fanoutt: " << triphones.back() << endl;
        }
    }

    cerr << "step 1" << endl;

    // All phone-phone combinations from one phone subwords to both fanout and fanin
    for (auto fphit = phones.begin(); fphit != phones.end(); ++fphit) {
        for (auto sphit = phones.begin(); sphit != phones.end(); ++sphit) {
            string fanint = string("_-") + string(1,*fphit) + string(1,'+') + string(1,*sphit);
            string fanoutt = string(1,*fphit) + string(1,'-') + string(1,*sphit) + string("+_");
            fanout[fanoutt] = -1;
            fanin[fanint] = -1;
            cerr << "fanint: " << fanint << endl;
            cerr << "fanoutt: " << fanoutt << endl;
        }
    }

    cerr << "step 2" << endl;

    // Fanin last triphone + phone from one phone subwords, all combinations to fanin
    for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
        for (auto phit = phones.begin(); phit != phones.end(); ++phit) {
            string fanint = string("_-") + string(1,(fiit->first)[2]) + string(1,'+') + string(1,*phit);
            fanin[fanint] = -1;
            cerr << "fanint: " << fanint << endl;
        }
    }

    cerr << "step 3" << endl;

    // Phone from one phone subwords + fanout first triphone, all combinations to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {
        for (auto phit = phones.begin(); phit != phones.end(); ++phit) {
            string fanoutt = string(1,(foit->first)[2]) + string(1,'-') + string(1,*phit) + string("+_");
            fanout[fanoutt] = -1;
            cerr << "fanoutt: " << fanoutt << endl;
        }
    }

    map<string, int> connected_fanin_nodes;
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        nodes.resize(nodes.size()+1);
        nodes.back().flags |= NODE_FAN_OUT_DUMMY;
        foit->second = nodes.size()-1;
        nodes[foit->second].label.assign(foit->first);
        int start_index = foit->second;

        // Connect one phone subwords right after the fanout for _-x+_ connector nodes
        /*
        if (foit->first[0] == '_' && foit->first[4] == '_')
        {
            set<int> temp_node_idxs;
            int dummy_node_idx = -1;
            for (auto opswit = one_phone_subwords.begin(); opswit != one_phone_subwords.end(); ++opswit) {
                if (dg.m_lexicon[*opswit][0] == foit->first) {
                    int temp_idx = connect_word(dg, nodes, *opswit, foit->second);
                    temp_node_idxs.insert(temp_idx);
                    if (dummy_node_idx == -1) {
                        dummy_node_idx = connect_dummy(nodes, temp_idx);
                        cerr << foit->first << " " << foit->second << endl;
                        cerr << "connected_dummy: " << temp_idx << ", " << dummy_node_idx << endl;
                    }
                    else
                        nodes[temp_idx].arcs.insert(dummy_node_idx);
                }
            }
            start_index = dummy_node_idx;
        }
        */

        for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
            string triphone1 = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + fiit->first[2];
            string triphone2 = foit->first[2] + string(1,'-') + fiit->first[2] + string(1,'+') + fiit->first[4];

            int tri1_idx = connect_triphone(dg, nodes, triphone1, start_index);
            int idx = connect_word(dg, nodes, "<w>", tri1_idx);
            idx = connect_triphone(dg, nodes, "_", idx);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end())
            {
                // Connect one phone subwords before second triphone for _-x+_ connector nodes in fanin
                /*
                if (fiit->first[0] == '_' && fiit->first[4] == '_')
                {
                    set<int> temp_node_idxs;
                    int dummy_node_idx = -1;
                    for (auto opswit = one_phone_subwords.begin(); opswit != one_phone_subwords.end(); ++opswit) {
                        if (dg.m_lexicon[*opswit][0] == fiit->first) {
                            int temp_idx = connect_word(dg, nodes, *opswit, idx);
                            temp_node_idxs.insert(temp_idx);
                            if (dummy_node_idx == -1)
                                dummy_node_idx = connect_dummy(nodes, temp_idx);
                            else
                                nodes[temp_idx].arcs.insert(dummy_node_idx);
                        }
                    }
                    idx = dummy_node_idx;
                }
                */

                idx = connect_triphone(dg, nodes, triphone2, idx);
                nodes[tri1_idx].arcs.insert(idx-2);
                connected_fanin_nodes[triphone2] = idx-2;
                if (fiit->second == -1) {
                    nodes.resize(nodes.size()+1);
                    nodes.back().flags |= NODE_FAN_IN_DUMMY;
                    fiit->second = nodes.size()-1;
                    nodes[fiit->second].label.assign(fiit->first);
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
    /*
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {
        cerr << "setting loop from fanout: " << foit->first << endl;
        //if (foit->first[0] == '_' && foit->first[4] == '_') continue;
        set<int> source_nodes;
        find_nodes_in_depth(nodes, source_nodes, 3, 0, foit->second);
        for (auto opswit = one_phone_subwords.begin(); opswit != one_phone_subwords.end(); ++opswit) {
            string single_phone = dg.m_lexicon[*opswit][0];
            string triphone = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + string(1,single_phone[2]);
            cerr << "\tconnecting triphone: " << triphone << endl;

            for (auto snit = source_nodes.begin(); snit != source_nodes.end(); ++snit) {
                int optidx = connect_word(dg, nodes, "<w>", *snit);
                optidx = connect_triphone(dg, nodes, "_", optidx);

                int lidx = connect_word(dg, nodes, *opswit, *snit);
                lidx = connect_triphone(dg, nodes, triphone, lidx);
                nodes[optidx].arcs.insert(lidx-2);

                string fanout_loop_connector = foit->first[2] + string(1,'-') + string(1,single_phone[2]) + string("+_");
                cerr << "\tto fanout: " << fanout_loop_connector << endl;
                if (fanout.find(fanout_loop_connector) == fanout.end()) {
                    cerr << "problem in connecting fanout loop for one phone subword: " << *opswit << endl;
                    assert(false);
                }
                nodes[lidx].arcs.insert(fanout[fanout_loop_connector]);
            }
        }
    }
    */

    // Add loops for one phone subwords from fanin back to fanin
    for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
        if (fiit->first[0] == '_' && fiit->first[4] == '_') continue;
        for (auto opswit = one_phone_subwords.begin(); opswit != one_phone_subwords.end(); ++opswit) {
            string single_phone = dg.m_lexicon[*opswit][0];
            string triphone = string(1,single_phone[2]) + string(1,'-') + fiit->first[2] + string(1,'+') + fiit->first[4];

            int lidx = connect_word(dg, nodes, *opswit, fiit->second);
            cerr << "connected from " << fiit->second << " to " << lidx << " with subword: " << *opswit <<  endl;
            lidx = connect_triphone(dg, nodes, triphone, lidx);
            string fanout_loop_connector = fiit->first[4] + string(1,'-') + string(1,single_phone[2]) + string("+_");
            if (fanout.find(fanout_loop_connector) == fanout.end()) {
                cerr << "problem in connecting fanout loop for one phone subword:" << *opswit << endl;
                assert(false);
            }
            nodes[lidx].arcs.insert(fanout[fanout_loop_connector]);
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


/*
void
connect_one_phone_subwords_from_start_to_cw(DecoderGraph &dg,
                                            set<string> &subwords,
                                            vector<DecoderGraph::Node> &nodes,
                                            map<string, int> &fanout)
{
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit)
        if (foit->first[0] == '_' && foit->first[4] == '_')
            nodes[START_NODE].arcs.insert(foit->second);
}


void
connect_one_phone_subwords_from_cw_to_end(DecoderGraph &dg,
                                          set<string> &subwords,
                                          vector<DecoderGraph::Node> &nodes,
                                          map<string, int> &fanin)
{
    for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit)
        if (fiit->first[0] == '_' && fiit->first[4] == '_')
            nodes[fiit->second].arcs.insert(END_NODE);
}
*/


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
        nodes[START_NODE].arcs.insert(fanout[fanoutt]);
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

