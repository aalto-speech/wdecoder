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

    // All phone-phone combinations from one phone subwords to fanout
    for (auto fphit = phones.begin(); fphit != phones.end(); ++fphit) {
        for (auto sphit = phones.begin(); sphit != phones.end(); ++sphit) {
            string fanint = string("_-") + string(1,*fphit) + string(1,'+') + string(1,*sphit);
            string fanoutt = string(1,*fphit) + string(1,'-') + string(1,*sphit) + string("+_");
            fanout[fanoutt] = -1;
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
        // Optional short silence and word break if not connecting out from _-x+_ connector
        for (auto opswit = one_phone_subwords.begin(); opswit != one_phone_subwords.end(); ++opswit) {

            string single_phone = dg.m_lexicon[*opswit][0];
            string triphone = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + string(1,single_phone[2]);

            int tridx = connect_triphone(dg, nodes, triphone, foit->second);
            string fanout_loop_connector = foit->first[2] + string(1,'-') + string(1,single_phone[2]) + string("+_");
            if (fanout.find(fanout_loop_connector) == fanout.end()) {
                cerr << "problem in connecting fanout loop for one phone subword:" << *opswit << endl;
                cerr << fanout_loop_connector << endl;
                assert(false);
            }

            // just subword in all cases
            int lidx = connect_word(dg, nodes, *opswit, tridx);
            nodes[lidx].arcs.insert(fanout[fanout_loop_connector]);

            // loops with word boundary only if not _-x+_ connector
            if (foit->first[0] != '_' || foit->first[4] != '_') {
                // optionally word boundary after subword
                int wbidx = connect_word(dg, nodes, "<w>", lidx);
                wbidx = connect_triphone(dg, nodes, "_", wbidx);

                // word boundary first then subword
                int l2idx = connect_word(dg, nodes, "<w>", tridx);
                l2idx = connect_triphone(dg, nodes, "_", l2idx);
                l2idx = connect_word(dg, nodes, *opswit, l2idx);

                nodes[wbidx].arcs.insert(fanout[fanout_loop_connector]);
                nodes[l2idx].arcs.insert(fanout[fanout_loop_connector]);
            }
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


void
create_forced_path(DecoderGraph &dg,
                   vector<DecoderGraph::Node> &nodes,
                   vector<string> &sentence)
{
    vector<string> word;
    vector<DecoderGraph::TriphoneNode> tnodes;
    tnodes.push_back(DecoderGraph::TriphoneNode(-1, dg.m_hmm_map["__"]));
    for (int i=2; i<(int)sentence.size()-1; i++) {
        if (sentence[i] == "<w>" && sentence[i-1] == "</s>")
            continue;
        else if (sentence[i] == "<w>") {
            vector<DecoderGraph::TriphoneNode> word_tnodes;
            triphonize(dg, word, word_tnodes);
            tnodes.insert(tnodes.end(), word_tnodes.begin(), word_tnodes.end());
            tnodes.insert(tnodes.begin()+(tnodes.size()-1), DecoderGraph::TriphoneNode(dg.m_subword_map["<w>"], -1));
            tnodes.push_back(DecoderGraph::TriphoneNode(-1, dg.m_hmm_map["__"]));
            word.clear();
        }
        else {
            word.push_back(sentence[i]);
        }
    }

    /*
    for (int t=0; t<(int)tnodes.size(); t++)
        if (tnodes[t].hmm_id != -1)
            cerr << dg.m_hmms[tnodes[t].hmm_id].label << " ";
        else
            cerr << "(" << dg.m_subwords[tnodes[t].subword_id] << ") ";
    cerr << endl;
    */

    nodes.clear();
    nodes.resize(1);
    int idx = 0;
    int crossword_start = -1;
    string crossword_left;
    string crossword_right;
    for (int t=0; t<(int)tnodes.size(); t++)
        if (tnodes[t].hmm_id != -1) {
            if (dg.m_hmms[tnodes[t].hmm_id].label.length() == 5
                && dg.m_hmms[tnodes[t].hmm_id].label[4] == '_') {
                crossword_start = idx;
                crossword_left = dg.m_hmms[tnodes[t].hmm_id].label;
            }

            idx = connect_triphone(dg, nodes, tnodes[t].hmm_id, idx);

            if (crossword_start != -1
                && dg.m_hmms[tnodes[t].hmm_id].label.length() == 5
                && dg.m_hmms[tnodes[t].hmm_id].label[0] == '_')
            {
                idx = connect_dummy(nodes, idx);
                crossword_right = dg.m_hmms[tnodes[t].hmm_id].label;
                crossword_left[4] = crossword_right[2];
                crossword_right[0] = crossword_left[2];
                //cerr << "left: " << crossword_left << endl;
                //cerr << "right: " << crossword_right << endl;
                int tmp = connect_triphone(dg, nodes, crossword_left, crossword_start);
                tmp = connect_triphone(dg, nodes, "_", tmp);
                tmp = connect_triphone(dg, nodes, crossword_right, tmp);
                nodes[tmp].arcs.insert(idx);
            }

        }
        else
            idx = connect_word(nodes, tnodes[t].subword_id, idx);
}


}

