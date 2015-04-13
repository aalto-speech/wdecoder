#include <cassert>

#include "gutils.hh"
#include "GraphBuilder.hh"

using namespace std;
using namespace gutils;


namespace wordgraphbuilder {


void
create_crossword_network(DecoderGraph &dg,
                         const set<string> &words,
                         vector<DecoderGraph::Node> &nodes,
                         map<string, int> &fanout,
                         map<string, int> &fanin)
{
    int spp = dg.m_states_per_phone;

    set<string> one_phone_subwords;
    set<char> phones;
    for (auto swit = dg.m_lexicon.begin(); swit != dg.m_lexicon.end(); ++swit) {
        if (words.find(swit->first) == words.end()) continue;
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

    // All phone-phone combinations from one phone words to fanout
    for (auto fphit = phones.begin(); fphit != phones.end(); ++fphit) {
        for (auto sphit = phones.begin(); sphit != phones.end(); ++sphit) {
            string fanint = string("_-") + string(1,*fphit) + string(1,'+') + string(1,*sphit);
            string fanoutt = string(1,*fphit) + string(1,'-') + string(1,*sphit) + string("+_");
            fanout[fanoutt] = -1;
        }
    }

    // Fanout last triphone + phone from one phone words, all combinations to fanout
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

            int idx = connect_triphone(dg, nodes, triphone1, start_index);
            idx = connect_triphone(dg, nodes, "_", idx);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end())
            {
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
connect_one_phone_words_from_start_to_cw(DecoderGraph &dg,
                                         const set<string> &words,
                                         vector<DecoderGraph::Node> &nodes,
                                         map<string, int> &fanout)
{
    for (auto wit = words.begin(); wit != words.end(); ++wit) {
        vector<string> &triphones = dg.m_lexicon[*wit];
        if (triphones.size() != 1 || triphones[0].length() == 1) continue;
        string fanoutt = triphones[0];
        int idx = connect_word(dg, nodes, *wit, START_NODE);
        if (fanout.find(fanoutt) == fanout.end()) {
            cerr << "problem in connecting: " << *wit << " from start to fanout" << endl;
            assert(false);
        }
        nodes[idx].arcs.insert(fanout[fanoutt]);
    }
}


void
connect_one_phone_words_from_cw_to_end(DecoderGraph &dg,
                                       const set<string> &words,
                                       vector<DecoderGraph::Node> &nodes,
                                       map<string, int> &fanin)
{
    for (auto wit = words.begin(); wit != words.end(); ++wit) {
        vector<string> &triphones = dg.m_lexicon[*wit];
        if (triphones.size() != 1 || triphones[0].length() == 1) continue;
        string fanint = triphones[0];
        if (fanin.find(fanint) == fanin.end()) {
            cerr << "problem in connecting: " << *wit << " fanin to end" << endl;
            assert(false);
        }
        int idx = connect_word(dg, nodes, *wit, fanin[fanint]);
        nodes[idx].arcs.insert(END_NODE);
    }
}


void
create_graph(DecoderGraph &dg,
             const set<string> &words,
             vector<DecoderGraph::Node> &nodes,
             bool verbose)
{
    nodes.clear();
    nodes.resize(2);

    for (auto wit = words.begin(); wit != words.end(); ++wit) {
        vector<TriphoneNode> word_triphones;
        triphonize_subword(dg, *wit, word_triphones);
        vector<DecoderGraph::Node> word_nodes;
        triphones_to_state_chain(dg, word_triphones, word_nodes);
        // One phone words without crossword path
        if (num_triphones(word_triphones) > 1) {
            word_nodes[3].from_fanin.insert(dg.m_lexicon[*wit][0]);
            word_nodes[word_nodes.size()-4].to_fanout.insert(dg.m_lexicon[*wit].back());
        }
        add_nodes_to_tree(dg, nodes, word_nodes);
    }
    lookahead_to_arcs(nodes);
    prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    wordgraphbuilder::create_crossword_network(dg, words, cw_nodes, fanout, fanin);
    if (verbose) cerr << "crossword network size: " << cw_nodes.size() << endl;
    minimize_crossword_network(cw_nodes, fanout, fanin);
    if (verbose) cerr << "tied crossword network size: " << cw_nodes.size() << endl;

    if (verbose) cerr << "Connecting crossword network.." << endl;
    graphbuilder::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);
    if (verbose) cerr << "number of hmm state nodes: " << reachable_graph_nodes(nodes) << endl;

    wordgraphbuilder::connect_one_phone_words_from_start_to_cw(dg, words, nodes, fanout);
    wordgraphbuilder::connect_one_phone_words_from_cw_to_end(dg, words, nodes, fanin);
}


void tie_graph(vector<DecoderGraph::Node> &nodes,
               bool verbose)
{
    if (verbose) cerr << "number of hmm state nodes: " << reachable_graph_nodes(nodes) << endl;

    if (verbose) cerr << endl;
    if (verbose) cerr << "Pushing word ids right.." << endl;
    push_word_ids_right(nodes);
    if (verbose) cerr << "Tying state prefixes.." << endl;
    tie_state_prefixes(nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

    if (verbose) cerr << endl;
    if (verbose) cerr << "Pushing word ids left.." << endl;
    push_word_ids_left(nodes);
    if (verbose) cerr << "Tying state suffixes.." << endl;
    tie_state_suffixes(nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

    //cerr << endl;
    //cerr << "Removing cw dummies.." << endl;
    //remove_cw_dummies(nodes);
    //cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

    if (verbose) cerr << endl;
    if (verbose) cerr << "Tying state suffixes.." << endl;
    tie_state_suffixes(nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

    //cerr << "Tying state prefixes.." << endl;
    //tie_state_prefixes(nodes);
    //cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
}


}

