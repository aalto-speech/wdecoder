#include <cassert>

#include "NoWBSubwordGraph.hh"

using namespace std;


NoWBSubwordGraph::NoWBSubwordGraph()
{
}


NoWBSubwordGraph::NoWBSubwordGraph(const set<string> &subwords,
                                   bool verbose)
{
    create_graph(subwords, verbose);
}


void
NoWBSubwordGraph::create_crossword_network(const set<string> &subwords,
                                           vector<DecoderGraph::Node> &nodes,
                                           map<string, int> &fanout,
                                           map<string, int> &fanin)
{
    int spp = m_states_per_phone;

    set<string> one_phone_subwords;
    set<char> phones;
    for (auto swit = m_lexicon.begin(); swit != m_lexicon.end(); ++swit) {
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

            int tri1_idx = connect_triphone(nodes, triphone1, start_index);
            int idx = connect_word(nodes, "<w>", tri1_idx);
            idx = connect_triphone(nodes, "_", idx);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end())
            {
                idx = connect_triphone(nodes, triphone2, idx);
                nodes[tri1_idx].arcs.insert(idx - (spp-1));
                connected_fanin_nodes[triphone2] = idx - (spp-1);
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

            string single_phone = m_lexicon[*opswit][0];
            string triphone = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + string(1,single_phone[2]);

            int tridx = connect_triphone(nodes, triphone, foit->second);
            string fanout_loop_connector = foit->first[2] + string(1,'-') + string(1,single_phone[2]) + string("+_");
            if (fanout.find(fanout_loop_connector) == fanout.end()) {
                cerr << "problem in connecting fanout loop for one phone subword:" << *opswit << endl;
                cerr << fanout_loop_connector << endl;
                assert(false);
            }

            // just subword in all cases
            int lidx = connect_word(nodes, *opswit, tridx);
            nodes[lidx].arcs.insert(fanout[fanout_loop_connector]);

            // loops with word boundary only if not _-x+_ connector
            if (foit->first[0] != '_' || foit->first[4] != '_') {
                // optionally word boundary after subword
                int wbidx = connect_word(nodes, "<w>", lidx);
                wbidx = connect_triphone(nodes, "_", wbidx);

                // word boundary first then subword
                int l2idx = connect_word(nodes, "<w>", tridx);
                l2idx = connect_triphone(nodes, "_", l2idx);
                l2idx = connect_word(nodes, *opswit, l2idx);

                nodes[wbidx].arcs.insert(fanout[fanout_loop_connector]);
                nodes[l2idx].arcs.insert(fanout[fanout_loop_connector]);
            }
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        cwnit->flags |= NODE_CW;
}


void
NoWBSubwordGraph::connect_one_phone_subwords_from_start_to_cw(const set<string> &subwords,
                                                              vector<DecoderGraph::Node> &nodes,
                                                              map<string, int> &fanout)
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        vector<string> &triphones = m_lexicon[*swit];
        if (triphones.size() != 1 || triphones[0].length() == 1) continue;
        string fanoutt = triphones[0];
        int idx = connect_word(nodes, *swit, START_NODE);
        if (fanout.find(fanoutt) == fanout.end()) {
            cerr << "problem in connecting: " << *swit << " from start to fanout" << endl;
            assert(false);
        }
        nodes[idx].arcs.insert(fanout[fanoutt]);
    }
}


void
NoWBSubwordGraph::connect_one_phone_subwords_from_cw_to_end(const set<string> &subwords,
                                                            vector<DecoderGraph::Node> &nodes,
                                                            map<string, int> &fanin)
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        vector<string> &triphones = m_lexicon[*swit];
        if (triphones.size() != 1 || triphones[0].length() == 1) continue;
        string fanint = triphones[0];
        if (fanin.find(fanint) == fanin.end()) {
            cerr << "problem in connecting: " << *swit << " fanin to end" << endl;
            assert(false);
        }
        int idx = connect_word(nodes, *swit, fanin[fanint]);
        nodes[idx].arcs.insert(END_NODE);
    }
}


void
NoWBSubwordGraph::create_graph(const set<string> &subwords,
                               bool verbose)
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        if (swit->find("<") != string::npos) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(*swit, subword_triphones);
        // One phone subwords not connected yet to the main tree
        if (num_triphones(subword_triphones) < 2) continue;
        vector<DecoderGraph::Node> subword_nodes;
        triphones_to_state_chain(subword_triphones, subword_nodes);
        subword_nodes[3].from_fanin.insert(m_lexicon[*swit][0]);
        subword_nodes[subword_nodes.size()-4].to_fanout.insert(m_lexicon[*swit].back());
        add_nodes_to_tree(m_nodes, subword_nodes);
    }
    lookahead_to_arcs(m_nodes);

    prune_unreachable_nodes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    if (verbose) cerr << "Creating crossword network.." << endl;
    create_crossword_network(subwords, cw_nodes, fanout, fanin);
    if (verbose) cerr << "crossword network size: " << cw_nodes.size() << endl;
    minimize_crossword_network(cw_nodes, fanout, fanin);
    if (verbose) cerr << "tied crossword network size: " << cw_nodes.size() << endl;

    if (verbose) cerr << "Connecting crossword network.." << endl;
    connect_crossword_network(cw_nodes, fanout, fanin, false);
    connect_end_to_start_node(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

    connect_one_phone_subwords_from_start_to_cw(subwords, m_nodes, fanout);
    connect_one_phone_subwords_from_cw_to_end(subwords, m_nodes, fanin);
    prune_unreachable_nodes(m_nodes);

    if (verbose) cerr << "Removing cw dummies.." << endl;
    remove_cw_dummies(m_nodes);

    if (verbose) cerr << "Tying nodes.." << endl;
    push_word_ids_right(m_nodes);
    tie_state_prefixes(m_nodes);
    tie_word_id_prefixes(m_nodes);
    tie_state_prefixes(m_nodes);
    tie_word_id_prefixes(m_nodes);
    tie_state_prefixes(m_nodes);

    push_word_ids_left(m_nodes);
    tie_state_suffixes(m_nodes);
    tie_word_id_suffixes(m_nodes);
    tie_state_suffixes(m_nodes);
    tie_word_id_suffixes(m_nodes);
    tie_state_suffixes(m_nodes);

    prune_unreachable_nodes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;
}

