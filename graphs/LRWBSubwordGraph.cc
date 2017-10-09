#include <cassert>

#include "LRWBSubwordGraph.hh"

using namespace std;

LRWBSubwordGraph::LRWBSubwordGraph()
{
}

LRWBSubwordGraph::LRWBSubwordGraph(const set<string> &prefix_subwords,
                                   const set<string> &stem_subwords,
                                   const set<string> &suffix_subwords,
                                   const set<string> &word_subwords,
                                   bool verbose)
{
    create_graph(prefix_subwords,
                 stem_subwords,
                 suffix_subwords,
                 word_subwords,
                 verbose);
}

void
LRWBSubwordGraph::read_lexicon(string lexfname,
                               set<string> &prefix_subwords,
                               set<string> &stem_subwords,
                               set<string> &suffix_subwords,
                               set<string> &word_subwords)
{
    read_noway_lexicon(lexfname);
    prefix_subwords.clear();
    stem_subwords.clear();
    suffix_subwords.clear();
    word_subwords.clear();

    for (auto swit = m_lexicon.begin(); swit != m_lexicon.end(); ++swit) {
        if (swit->first.find("<") != string::npos) continue;
        if (swit->first.length() == 0) continue;

        if (swit->first.front() == '_' && swit->first.back() == '_')
            word_subwords.insert(swit->first);
        else if (swit->first.front() == '_')
            prefix_subwords.insert(swit->first);
        else if (swit->first.back() == '_')
            suffix_subwords.insert(swit->first);
        else
            stem_subwords.insert(swit->first);
    }

    cerr << "Number of prefix subwords: " << prefix_subwords.size() << endl;
    cerr << "Number of stem subwords: " << stem_subwords.size() << endl;
    cerr << "Number of suffix subwords: " << suffix_subwords.size() << endl;
    cerr << "Number of word subwords: " << word_subwords.size() << endl;
}

void
LRWBSubwordGraph::create_crossword_network(const set<string> &words,
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
LRWBSubwordGraph::connect_one_phone_words_from_start_to_cw(const set<string> &words,
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
LRWBSubwordGraph::connect_one_phone_words_from_cw_to_end(const set<string> &words,
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
LRWBSubwordGraph::create_graph(const set<string> &prefix_subwords,
                               const set<string> &stem_subwords,
                               const set<string> &suffix_subwords,
                               const set<string> &word_subwords,
                               bool verbose)
{
    for (auto wit = word_subwords.begin(); wit != word_subwords.end(); ++wit) {
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
    create_crossword_network(word_subwords, cw_nodes, fanout, fanin);
    if (verbose) cerr << "crossword network size: " << cw_nodes.size() << endl;
    minimize_crossword_network(cw_nodes, fanout, fanin);
    if (verbose) cerr << "tied crossword network size: " << cw_nodes.size() << endl;

    if (verbose) cerr << "Connecting crossword network.." << endl;
    connect_crossword_network(cw_nodes, fanout, fanin);
    connect_end_to_start_node(m_nodes);
    if (verbose) cerr << "number of hmm state nodes: " << reachable_graph_nodes(m_nodes) << endl;

    connect_one_phone_words_from_start_to_cw(word_subwords, m_nodes, fanout);
    connect_one_phone_words_from_cw_to_end(word_subwords, m_nodes, fanin);
}

