#include <cassert>

#include "gutils.hh"

using namespace std;
using namespace gutils;


namespace graphbuilder {


void
create_crossword_network(DecoderGraph &dg,
                         const map<string, vector<string> > &word_segs,
                         vector<DecoderGraph::Node> &nodes,
                         map<string, int> &fanout,
                         map<string, int> &fanin,
                         bool wb_symbol_in_middle)
{
    int spp = dg.m_states_per_phone;

    int error_count = 0;
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit) {
        vector<string> triphones;
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit) {
            for (auto tit = dg.m_lexicon[*swit].begin(); tit != dg.m_lexicon[*swit].end(); ++tit) {
                triphones.push_back(*tit);
            }
        }
        if (triphones.size() < 2) {
            error_count++;
            continue;
        }
        string fanint = string("_-") + triphones[0][2] + string(1,'+') + triphones[1][2];
        string fanoutt = triphones[triphones.size()-2][2] + string(1,'-') + triphones[triphones.size()-1][2] + string("+_");
        fanout[fanoutt] = -1;
        fanin[fanint] = -1;
    }
    if (error_count > 0) cerr << error_count << " words with less than two phones." << endl;

    map<string, int> connected_fanin_nodes;
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        nodes.resize(nodes.size()+1);
        nodes.back().flags |= NODE_FAN_OUT_DUMMY;
        foit->second = nodes.size()-1;

        for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
            string triphone1 = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + fiit->first[2];
            string triphone2 = foit->first[2] + string(1,'-') + fiit->first[2] + string(1,'+') + fiit->first[4];

            int idx = connect_triphone(dg, nodes, triphone1, foit->second);
            if (wb_symbol_in_middle) idx = connect_word(dg, nodes, "<w>", idx);
            idx = connect_triphone(dg, nodes, "_", idx);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end()) {
                idx = connect_triphone(dg, nodes, triphone2, idx);
                connected_fanin_nodes[triphone2] = idx - (spp-1);
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
        cwnit->flags |= NODE_CW;
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

    for (unsigned int i=0; i<nodes.size(); i++) {
        DecoderGraph::Node &nd = nodes[i];
        for (auto ffi=nd.from_fanin.begin(); ffi!=nd.from_fanin.end(); ++ffi)
            nodes[fanin[*ffi]].arcs.insert(i);
        nd.from_fanin.clear();
    }

    if (push_left_after_fanin)
        push_word_ids_left(nodes);
    else
        set_reverse_arcs_also_from_unreachable(nodes);

    for (unsigned int i=0; i<nodes.size(); i++) {
        DecoderGraph::Node &nd = nodes[i];
        for (auto tfo=nd.to_fanout.begin(); tfo!=nd.to_fanout.end(); ++tfo)
            nd.arcs.insert(fanout[*tfo]);
        nd.to_fanout.clear();
    }
}


void create_graph(DecoderGraph &dg,
                  vector<DecoderGraph::Node> &nodes,
                  const map<string, vector<string> > word_segs,
                  bool wb_symbol,
                  bool connect_cw_network)
{
    nodes.clear();
    nodes.resize(2);

    int triphonize_error = 0;
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit) {
        vector<TriphoneNode> word_triphones;
        bool ok = triphonize(dg, wit->second, word_triphones);
        if (!ok) {
            triphonize_error++;
            continue;
        }

        if (num_triphones(word_triphones) < 2) {
            cerr << "One phone words not supported at the moment" << endl;
            exit(1);
        }

        vector<DecoderGraph::Node> word_nodes;
        triphones_to_state_chain(dg, word_triphones, word_nodes);

        bool first_assigned = false;
        string first_triphone;
        string last_triphone;
        for (unsigned int i=0; i< word_triphones.size(); i++) {
            if (word_triphones[i].hmm_id == -1) continue;
            string triphone_label = dg.m_hmms[word_triphones[i].hmm_id].label;
            if (!first_assigned) {
                first_triphone.assign(triphone_label);
                first_assigned = true;
            }
            last_triphone.assign(triphone_label);
        }
        word_nodes[3].from_fanin.insert(first_triphone);
        word_nodes[word_nodes.size()-4].to_fanout.insert(last_triphone);

        add_nodes_to_tree(dg, nodes, word_nodes);
    }
    lookahead_to_arcs(nodes);
    if (triphonize_error > 0) cerr << triphonize_error << " words could not be triphonized." << endl;

    if (connect_cw_network) {
        prune_unreachable_nodes(nodes);
        cerr << "number of hmm state nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << "Creating crossword network.." << endl;
        vector<DecoderGraph::Node> cw_nodes;
        map<string, int> fanout, fanin;
        create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin, wb_symbol);
        cerr << "crossword network size: " << cw_nodes.size() << endl;
        minimize_crossword_network(cw_nodes, fanout, fanin);
        cerr << "tied crossword network size: " << cw_nodes.size() << endl;

        connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin, false);
        connect_end_to_start_node(nodes);
        cerr << "number of hmm state nodes: " << reachable_graph_nodes(nodes) << endl;
    }
}


};

