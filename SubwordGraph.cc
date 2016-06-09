#include <cassert>

#include "SubwordGraph.hh"

using namespace std;


SubwordGraph::SubwordGraph()
{

}


SubwordGraph::SubwordGraph(const std::set<std::string> &subwords,
                           bool verbose)
{
    create_graph(subwords, verbose);
}


void
SubwordGraph::create_crossword_network(const set<string> &subwords,
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
        if (triphones.size() > 1) {
            fanout[triphones.back()] = -1;
            fanin[triphones[0]] = -1;
        }
        else if (triphones.size() == 1 && is_triphone(triphones[0])) {
            one_phone_subwords.insert(swit->first);
            phones.insert(tphone(triphones[0]));
            fanout[triphones[0]] = -1;
            fanin[triphones[0]] = -1;
        }
    }

    // All phone-phone combinations from one phone subwords to fanout
    for (auto fphit = phones.begin(); fphit != phones.end(); ++fphit) {
        for (auto sphit = phones.begin(); sphit != phones.end(); ++sphit) {
            string fanoutt = construct_triphone(*fphit, *sphit, SIL_CTXT);
            fanout[fanoutt] = -1;
        }
    }

    // Fanout last triphone + phone from one phone subwords, all combinations to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {
        if (tlc(foit->first) == SIL_CTXT) continue;
        for (auto phit = phones.begin(); phit != phones.end(); ++phit) {
            string fanoutt = construct_triphone(tphone(foit->first), *phit, SIL_CTXT);
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
            string triphone1 = construct_triphone(tlc(foit->first), tphone(foit->first), tphone(fiit->first));
            string triphone2 = construct_triphone(tphone(foit->first), tphone(fiit->first), trc(fiit->first));

            int tri1_idx = connect_triphone(nodes, triphone1, start_index);
            int idx = connect_word(nodes, "<w>", tri1_idx);
            idx = connect_triphone(nodes, SHORT_SIL, idx);

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
            string triphone = construct_triphone(tlc(foit->first), tphone(foit->first), tphone(single_phone));
            string fanout_loop_connector = construct_triphone(tphone(foit->first), tphone(single_phone), SIL_CTXT);
            if (fanout.find(fanout_loop_connector) == fanout.end()) {
                cerr << "problem in connecting fanout loop for one phone subword:" << *opswit << endl;
                cerr << fanout_loop_connector << endl;
                assert(false);
            }

            int tridx = connect_triphone(nodes, triphone, foit->second);

            // just subword in all cases
            int lidx = connect_word(nodes, *opswit, tridx);
            nodes[lidx].arcs.insert(fanout[fanout_loop_connector]);

            // loops with word boundary only if not _-x+_ source connector
            if (tlc(foit->first) != SIL_CTXT || trc(foit->first) != SIL_CTXT) {
                int l2idx = connect_word(nodes, "<w>", tridx);
                l2idx = connect_triphone(nodes, SHORT_SIL, l2idx);
                l2idx = connect_word(nodes, *opswit, l2idx);
                nodes[l2idx].arcs.insert(fanout[fanout_loop_connector]);
            }
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        cwnit->flags |= NODE_CW;
}


void
SubwordGraph::connect_one_phone_subwords_from_start_to_cw(const set<string> &subwords,
                                                          vector<DecoderGraph::Node> &nodes,
                                                          map<string, int> &fanout)
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        vector<string> &triphones = m_lexicon[*swit];
        if (triphones.size() != 1) continue;
        if (!is_triphone(triphones[0])) continue;
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
SubwordGraph::connect_one_phone_subwords_from_cw_to_end(const set<string> &subwords,
                                                        vector<DecoderGraph::Node> &nodes,
                                                        map<string, int> &fanin)
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        vector<string> &triphones = m_lexicon[*swit];
        if (triphones.size() != 1) continue;
        if (!is_triphone(triphones[0])) continue;
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
SubwordGraph::create_forced_path(vector<DecoderGraph::Node> &nodes,
                                 vector<string> &sentence,
                                 map<int, string> &node_labels)
{
    vector<string> word;
    vector<TriphoneNode> tnodes;

    // Create initial triphone graph
    tnodes.push_back(TriphoneNode(-1, m_hmm_map[LONG_SIL]));
    for (int i=2; i<(int)sentence.size()-1; i++) {
        if (sentence[i] == "<w>" && sentence[i-1] == "</s>")
            continue;
        else if (sentence[i] == "<w>") {
            vector<TriphoneNode> word_tnodes;
            triphonize(word, word_tnodes);
            tnodes.insert(tnodes.end(), word_tnodes.begin(), word_tnodes.end());
            tnodes.insert(tnodes.begin()+(tnodes.size()-1), TriphoneNode(m_subword_map["<w>"], -1));
            tnodes.push_back(TriphoneNode(-1, m_hmm_map[LONG_SIL]));
            word.clear();
        }
        else
            word.push_back(sentence[i]);
    }

    // Convert to HMM states, also with crossword context
    nodes.clear(); nodes.resize(1);
    int idx = 0, crossword_start = -1;
    string crossword_left, crossword_right, label;
    node_labels.clear();

    for (int t=0; t<(int)tnodes.size(); t++)
        if (tnodes[t].hmm_id != -1) {

            if (is_triphone(m_hmms[tnodes[t].hmm_id].label) &&
                trc(m_hmms[tnodes[t].hmm_id].label) == SIL_CTXT)
            {
                crossword_start = idx;
                crossword_left = m_hmms[tnodes[t].hmm_id].label;
            }

            idx = connect_triphone(nodes, tnodes[t].hmm_id, idx, node_labels);

            if (crossword_start != -1 &&
                is_triphone(m_hmms[tnodes[t].hmm_id].label) &&
                tlc(m_hmms[tnodes[t].hmm_id].label) == SIL_CTXT)
            {
                idx = connect_dummy(nodes, idx);

                crossword_right = m_hmms[tnodes[t].hmm_id].label;
                crossword_left[4] = crossword_right[2];
                crossword_right[0] = crossword_left[2];

                int tmp = connect_triphone(nodes, crossword_left, crossword_start, node_labels);
                tmp = connect_triphone(nodes, SHORT_SIL, tmp, node_labels);
                tmp = connect_triphone(nodes, crossword_right, tmp, node_labels);

                nodes[tmp].arcs.insert(idx);
            }

        }
        else
            idx = connect_word(nodes, tnodes[t].subword_id, idx);
}


void
SubwordGraph::create_graph(const set<string> &subwords,
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


// Subword graph with word boundaries as below
// <s> sw sw <w> sw sw <w> sw sw sw </s>
void
SubwordGraph::add_long_silence_no_start_end_wb()
{
    m_nodes[END_NODE].arcs.clear();

    int ls_len = m_hmms[m_hmm_map[LONG_SIL]].states.size() - 2;

    node_idx_t node_idx = END_NODE;
    node_idx = connect_triphone(m_nodes, LONG_SIL, node_idx, NODE_SILENCE);
    node_idx = connect_word(m_nodes, "</s>", node_idx);
    node_idx = connect_triphone(m_nodes, LONG_SIL, node_idx, NODE_SILENCE);
    m_nodes[node_idx].arcs.insert(START_NODE);
    m_nodes[node_idx-ls_len].arcs.insert(START_NODE);
    m_nodes[node_idx-(ls_len-1)].flags |= NODE_DECODE_START;

    node_idx = END_NODE;
    node_idx = connect_triphone(m_nodes, SHORT_SIL, node_idx, NODE_SILENCE);
    node_idx = connect_word(m_nodes, "<w>", node_idx);
    m_nodes[node_idx].arcs.insert(START_NODE);
}
