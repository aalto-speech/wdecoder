#include <cassert>

#include "GraphBuilder.hh"

using namespace std;


SWWGraph::SWWGraph()
{

}


SWWGraph::SWWGraph(const map<string, vector<string> > &word_segs,
                   bool wb_symbol,
                   bool connect_cw_network,
                   bool verbose)
{
    create_graph(word_segs, wb_symbol, connect_cw_network, verbose);
}


void
SWWGraph::create_crossword_network(const map<string, vector<string> > &word_segs,
                                   vector<DecoderGraph::Node> &nodes,
                                   map<string, int> &fanout,
                                   map<string, int> &fanin,
                                   bool wb_symbol_in_middle)
{
    int spp = m_states_per_phone;

    int error_count = 0;
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit) {
        vector<string> triphones;
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit) {
            for (auto tit = m_lexicon[*swit].begin(); tit != m_lexicon[*swit].end(); ++tit) {
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

            int idx = connect_triphone(nodes, triphone1, foit->second);
            if (wb_symbol_in_middle) idx = connect_word(nodes, "<w>", idx);
            idx = connect_triphone(nodes, "_", idx);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end()) {
                idx = connect_triphone(nodes, triphone2, idx);
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
SWWGraph::create_graph(const map<string, vector<string> > &word_segs,
                       bool wb_symbol,
                       bool connect_cw_network,
                       bool verbose)
{
    int triphonize_error = 0;
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit) {

        if (wit->first.find("<") != string::npos) continue;

        vector<TriphoneNode> word_triphones;
        bool ok = triphonize(wit->second, word_triphones);
        if (!ok) {
            triphonize_error++;
            continue;
        }

        if (num_triphones(word_triphones) < 2) {
            cerr << "Skipping one phone word " << wit->first << endl;
            continue;
        }

        vector<DecoderGraph::Node> word_nodes;
        triphones_to_state_chain(word_triphones, word_nodes);

        bool first_assigned = false;
        string first_triphone;
        string last_triphone;
        for (unsigned int i=0; i< word_triphones.size(); i++) {
            if (word_triphones[i].hmm_id == -1) continue;
            string triphone_label = m_hmms[word_triphones[i].hmm_id].label;
            if (!first_assigned) {
                first_triphone.assign(triphone_label);
                first_assigned = true;
            }
            last_triphone.assign(triphone_label);
        }
        word_nodes[3].from_fanin.insert(first_triphone);
        word_nodes[word_nodes.size()-4].to_fanout.insert(last_triphone);

        add_nodes_to_tree(m_nodes, word_nodes);
    }
    lookahead_to_arcs(m_nodes);
    if (triphonize_error > 0) cerr << triphonize_error << " words could not be triphonized." << endl;

    if (connect_cw_network) {
        prune_unreachable_nodes(m_nodes);
        if (verbose) cerr << "number of hmm state nodes: " << reachable_graph_nodes(m_nodes) << endl;

        if (verbose) cerr << "Creating crossword network.." << endl;
        vector<DecoderGraph::Node> cw_nodes;
        map<string, int> fanout, fanin;
        create_crossword_network(word_segs, cw_nodes, fanout, fanin, wb_symbol);
        if (verbose) cerr << "crossword network size: " << cw_nodes.size() << endl;
        minimize_crossword_network(cw_nodes, fanout, fanin);
        if (verbose) cerr << "tied crossword network size: " << cw_nodes.size() << endl;

        connect_crossword_network(cw_nodes, fanout, fanin, false);
        connect_end_to_start_node(m_nodes);
        if (verbose) cerr << "number of hmm state nodes: " << reachable_graph_nodes(m_nodes) << endl;
    }

    prune_unreachable_nodes(m_nodes);
}


void
SWWGraph::tie_graph(bool no_push,
                    bool verbose)
{
    if (verbose) cerr << "Tying state suffixes.." << endl;
    tie_state_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;
    if (verbose) cerr << "Tying word id suffixes.." << endl;
    tie_word_id_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

    if (verbose) cerr << endl;
    if (verbose) cerr << "Tying state suffixes.." << endl;
    tie_state_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;
    if (verbose) cerr << "Tying word id suffixes.." << endl;
    tie_word_id_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

    if (verbose) cerr << endl;
    if (verbose) cerr << "Tying state suffixes.." << endl;
    tie_state_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;
    if (verbose) cerr << "Tying word id suffixes.." << endl;
    tie_word_id_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

    if (verbose) cerr << endl;
    if (verbose) cerr << "Tying state suffixes.." << endl;
    tie_state_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;
    if (verbose) cerr << "Tying word id suffixes.." << endl;
    tie_word_id_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

    if (verbose) cerr << endl;
    if (verbose) cerr << "Tying state suffixes.." << endl;
    tie_state_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;
    if (verbose) cerr << "Tying word id suffixes.." << endl;
    tie_word_id_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

    if (verbose) cerr << endl;
    if (verbose) cerr << "Removing cw dummies.." << endl;
    remove_cw_dummies(m_nodes);
    if (verbose) cerr << "Tying state prefixes.." << endl;
    tie_state_prefixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;
    if (verbose) cerr << "Tying state suffixes.." << endl;
    tie_state_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

    if (!no_push) {
        if (verbose) cerr << "Pushing subword ids right.." << endl;
        push_word_ids_right(m_nodes);
        if (verbose) cerr << "Tying state prefixes.." << endl;
        tie_state_prefixes(m_nodes);
        if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

        if (verbose) cerr << "Pushing subword ids left.." << endl;
        push_word_ids_left(m_nodes);
        if (verbose) cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(m_nodes);
        if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;
    }
}

