#include <cassert>

#include "WordGraph.hh"

using namespace std;


WordGraph::WordGraph()
{

}


WordGraph::WordGraph(const std::set<std::string> &words,
                     bool verbose)
{
    create_graph(words, verbose);
}


void
WordGraph::create_crossword_network(const set<string> &words,
                                    vector<DecoderGraph::Node> &nodes,
                                    map<string, int> &fanout,
                                    map<string, int> &fanin)
{
    int spp = m_states_per_phone;

    set<string> one_phone_subwords;
    set<char> phones;
    for (auto swit = m_lexicon.begin(); swit != m_lexicon.end(); ++swit) {
        if (words.find(swit->first) == words.end()) continue;
        vector<string> &triphones = swit->second;
        if (triphones.size() == 0) continue;
        else if (triphones.size() == 1 && is_triphone(triphones[0])) {
            one_phone_subwords.insert(swit->first);
            phones.insert(tphone(triphones[0]));
            fanout[triphones[0]] = -1;
            fanin[triphones[0]] = -1;
        }
        else if (triphones.size() > 1) {
            fanout[triphones.back()] = -1;
            fanin[triphones[0]] = -1;
        }
    }

    // All phone-phone combinations from one phone words to fanout
    for (auto fphit = phones.begin(); fphit != phones.end(); ++fphit)
        for (auto sphit = phones.begin(); sphit != phones.end(); ++sphit) {
            string fanoutt = construct_triphone(*fphit, *sphit, SIL_CTXT);
            fanout[fanoutt] = -1;
        }

    // Fanout last triphone + phone from one phone words, all combinations to fanout
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

            int idx = connect_triphone(nodes, triphone1, start_index);
            idx = connect_triphone(nodes, SHORT_SIL, idx);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end())
            {
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
WordGraph::connect_one_phone_words_from_start_to_cw(const set<string> &words,
                                                    vector<DecoderGraph::Node> &nodes,
                                                    map<string, int> &fanout)
{
    for (auto wit = words.begin(); wit != words.end(); ++wit) {
        vector<string> &triphones = m_lexicon[*wit];
        if (triphones.size() != 1 || !is_triphone(triphones[0])) continue;
        string fanoutt = triphones[0];
        int idx = connect_word(nodes, *wit, START_NODE);
        if (fanout.find(fanoutt) == fanout.end()) {
            cerr << "problem in connecting: " << *wit << " from start to fanout" << endl;
            assert(false);
        }
        nodes[idx].arcs.insert(fanout[fanoutt]);
    }
}


void
WordGraph::connect_one_phone_words_from_cw_to_end(const set<string> &words,
                                                  vector<DecoderGraph::Node> &nodes,
                                                  map<string, int> &fanin)
{
    for (auto wit = words.begin(); wit != words.end(); ++wit) {
        vector<string> &triphones = m_lexicon[*wit];
        if (triphones.size() != 1 || !is_triphone(triphones[0])) continue;
        string fanint = triphones[0];
        if (fanin.find(fanint) == fanin.end()) {
            cerr << "problem in connecting: " << *wit << " fanin to end" << endl;
            assert(false);
        }
        int idx = connect_word(nodes, *wit, fanin[fanint]);
        nodes[idx].arcs.insert(END_NODE);
    }
}


void
WordGraph::create_graph(const set<string> &words,
                        bool verbose)
{
    for (auto wit = words.begin(); wit != words.end(); ++wit) {
        if (wit->find("<") != string::npos) continue;
        vector<TriphoneNode> word_triphones;
        triphonize_subword(*wit, word_triphones);
        vector<DecoderGraph::Node> word_nodes;
        triphones_to_state_chain(word_triphones, word_nodes);
        // One phone words without crossword path
        if (num_triphones(word_triphones) > 1) {
            word_nodes[3].from_fanin.insert(m_lexicon[*wit][0]);
            word_nodes[word_nodes.size()-4].to_fanout.insert(m_lexicon[*wit].back());
        }
        add_nodes_to_tree(m_nodes, word_nodes);
    }
    lookahead_to_arcs(m_nodes);
    prune_unreachable_nodes(m_nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    create_crossword_network(words, cw_nodes, fanout, fanin);
    if (verbose) cerr << "crossword network size: " << cw_nodes.size() << endl;
    minimize_crossword_network(cw_nodes, fanout, fanin);
    if (verbose) cerr << "tied crossword network size: " << cw_nodes.size() << endl;

    if (verbose) cerr << "Connecting crossword network.." << endl;
    connect_crossword_network(cw_nodes, fanout, fanin);
    connect_end_to_start_node(m_nodes);
    if (verbose) cerr << "number of hmm state nodes: " << reachable_graph_nodes(m_nodes) << endl;

    connect_one_phone_words_from_start_to_cw(words, m_nodes, fanout);
    connect_one_phone_words_from_cw_to_end(words, m_nodes, fanin);
}


void
WordGraph::tie_graph(bool verbose,
                     bool remove_cw_markers)
{
    if (verbose) cerr << "number of hmm state nodes: " << reachable_graph_nodes(m_nodes) << endl;

    if (verbose) cerr << endl;
    if (verbose) cerr << "Pushing word ids right.." << endl;
    push_word_ids_right(m_nodes);
    if (verbose) cerr << "Tying state prefixes.." << endl;
    tie_state_prefixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

    if (verbose) cerr << endl;
    if (verbose) cerr << "Pushing word ids left.." << endl;
    push_word_ids_left(m_nodes);
    if (verbose) cerr << "Tying state suffixes.." << endl;
    tie_state_suffixes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

    if (remove_cw_markers) {
        cerr << endl;
        cerr << "Removing cw dummies.." << endl;
        remove_cw_dummies(m_nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

        if (verbose) cerr << endl;
        if (verbose) cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(m_nodes);
        if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

        cerr << "Tying state prefixes.." << endl;
        tie_state_prefixes(m_nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;
    }
}

