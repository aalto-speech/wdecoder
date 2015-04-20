#include <cassert>

#include "NoWBSubwordGraph.hh"

using namespace std;


NoWBSubwordGraph::NoWBSubwordGraph()
{
}


NoWBSubwordGraph::NoWBSubwordGraph(const set<string> &word_start_subwords,
                                   const set<string> &subwords,
                                   bool verbose)
{
    create_graph(word_start_subwords, subwords, verbose);
}


void
NoWBSubwordGraph::create_crossword_network(const set<string> &fanout_subwords,
                                           const set<string> &fanin_subwords,
                                           vector<DecoderGraph::Node> &nodes,
                                           map<string, int> &fanout,
                                           map<string, int> &fanin)
{
    int spp = m_states_per_phone;

    set<string> one_phone_subwords;
    set<char> phones;
    for (auto swit = m_lexicon.begin(); swit != m_lexicon.end(); ++swit) {
        if (fanout_subwords.find(swit->first) == fanout_subwords.end()) continue;
        vector<string> &triphones = swit->second;
        if (triphones.size() == 0) continue;
        else if (triphones.size() == 1 && triphones[0].length() == 5) {
            one_phone_subwords.insert(swit->first);
            phones.insert(triphones[0][2]);
            fanout[triphones[0]] = -1;
        }
        else if (triphones.size() > 1)
            fanout[triphones.back()] = -1;
    }

    for (auto swit = m_lexicon.begin(); swit != m_lexicon.end(); ++swit) {
        if (fanin_subwords.find(swit->first) == fanin_subwords.end()) continue;
        vector<string> &triphones = swit->second;
        if (triphones.size() == 0) continue;
        else if (triphones.size() == 1 && triphones[0].length() == 5) {
            one_phone_subwords.insert(swit->first);
            phones.insert(triphones[0][2]);
            fanin[triphones[0]] = -1;
        }
        else if (triphones.size() > 1)
            fanin[triphones[0]] = -1;
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

            int idx = connect_triphone(nodes, triphone1, start_index);

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
NoWBSubwordGraph::create_graph(const set<string> &word_start_subwords,
                               const set<string> &subwords,
                               bool verbose)
{
    std::vector<DecoderGraph::Node> word_start_nodes(2);
    for (auto swit = word_start_subwords.begin(); swit != word_start_subwords.end(); ++swit) {
        if (swit->find("<") != string::npos) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(*swit, subword_triphones);
        // One phone subwords not connected yet to the main tree
        if (num_triphones(subword_triphones) < 2) continue;
        vector<DecoderGraph::Node> subword_nodes;
        triphones_to_state_chain(subword_triphones, subword_nodes);
        //subword_nodes[3].from_fanin.insert(m_lexicon[*swit][0]);
        subword_nodes[subword_nodes.size()-4].to_fanout.insert(m_lexicon[*swit].back());
        add_nodes_to_tree(word_start_nodes, subword_nodes);
    }
    lookahead_to_arcs(word_start_nodes);

    prune_unreachable_nodes(word_start_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(word_start_nodes) << endl;

    std::vector<DecoderGraph::Node> nodes(2);
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        if (swit->find("<") != string::npos) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(*swit, subword_triphones);
        // One phone subwords not connected yet to the main tree
        if (num_triphones(subword_triphones) < 2) continue;
        vector<DecoderGraph::Node> subword_nodes;
        triphones_to_state_chain(subword_triphones, subword_nodes);
        subword_nodes[3].from_fanin.insert(m_lexicon[*swit][0]);
        //subword_nodes[subword_nodes.size()-4].to_fanout.insert(m_lexicon[*swit].back());
        add_nodes_to_tree(nodes, subword_nodes);
    }
    lookahead_to_arcs(nodes);

    offset(nodes, word_start_nodes.size());
    word_start_nodes[END_NODE].arcs.insert(START_NODE);
    nodes[END_NODE].arcs.insert(START_NODE);

    word_start_nodes.insert(word_start_nodes.end(), nodes.begin(), nodes.end());

    map<string, int> fanout;
    map<string, int> fanin;
    vector<DecoderGraph::Node> cw_nodes;
    create_crossword_network(word_start_subwords, subwords, cw_nodes, fanout, fanin);
    m_nodes.swap(word_start_nodes);
    connect_crossword_network(cw_nodes, fanout, fanin, false);

    prune_unreachable_nodes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

}


void
NoWBSubwordGraph::offset(vector<DecoderGraph::Node> &nodes,
                         int offset)
{
    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit) {
        std::set<unsigned int> arcs;
        for (auto ait = nit->arcs.begin(); ait != nit->arcs.end(); ++ait)
            arcs.insert(*ait + offset);
        nit->arcs.swap(arcs);

        std::set<unsigned int> rev_arcs;
        for (auto ait = nit->reverse_arcs.begin(); ait != nit->reverse_arcs.end(); ++ait)
            rev_arcs.insert(*ait + offset);
        nit->reverse_arcs.swap(rev_arcs);
    }
}

