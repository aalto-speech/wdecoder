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
NoWBSubwordGraph::create_crossword_network(std::vector<std::pair<unsigned int, std::string> > &fanout_triphones,
                                           std::vector<std::pair<unsigned int, std::string> > &fanin_triphones,
                                           vector<DecoderGraph::Node> &nodes,
                                           map<string, int> &fanout,
                                           map<string, int> &fanin)
{
    int spp = m_states_per_phone;

    set<string> one_phone_subwords;
    set<char> phones;
    for (auto foit=fanout_triphones.begin(); foit != fanout_triphones.end(); ++foit)
        fanout[foit->second] = -1;

    for (auto fiit=fanin_triphones.begin(); fiit != fanin_triphones.end(); ++fiit)
        fanin[fiit->second] = -1;

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
NoWBSubwordGraph::connect_crossword_network(vector<DecoderGraph::Node> &nodes,
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


void
NoWBSubwordGraph::connect_crossword_network(vector<DecoderGraph::Node> &nodes,
                                            vector<pair<unsigned int, string> > &fanout_connectors,
                                            vector<pair<unsigned int, string> > &fanin_connectors,
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

    for (auto ficit = fanin_connectors.begin(); ficit != fanin_connectors.end(); ++ficit) {
        DecoderGraph::Node &nd = nodes[ficit->first];
        nodes[fanin[ficit->second]].arcs.insert(ficit->first);
    }

    if (push_left_after_fanin)
        push_word_ids_left(nodes);
    else
        set_reverse_arcs_also_from_unreachable(nodes);

    for (auto focit = fanout_connectors.begin(); focit != fanout_connectors.end(); ++focit) {
        DecoderGraph::Node &nd = nodes[focit->first];
        nd.arcs.insert(fanout[focit->second]);
    }

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
NoWBSubwordGraph::create_graph(const set<string> &prefix_subwords,
                               const set<string> &suffix_subwords,
                               bool verbose)
{
    vector<DecoderGraph::Node> prefix_nodes(2);
    for (auto swit = prefix_subwords.begin(); swit != prefix_subwords.end(); ++swit) {
        if (swit->find("<") != string::npos) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(*swit, subword_triphones);
        // One phone subwords not connected yet to the main tree
        if (num_triphones(subword_triphones) < 2) continue;
        vector<DecoderGraph::Node> subword_nodes;
        triphones_to_state_chain(subword_triphones, subword_nodes);
        subword_nodes[3].from_fanin.insert(m_lexicon[*swit][0]);
        subword_nodes[subword_nodes.size()-4].to_fanout.insert(m_lexicon[*swit].back());
        add_nodes_to_tree(prefix_nodes, subword_nodes);
    }
    lookahead_to_arcs(prefix_nodes);

    vector<pair<unsigned int, string> > prefix_fanout_connectors, prefix_fanin_connectors;
    collect_crossword_connectors(prefix_nodes, prefix_fanout_connectors, prefix_fanin_connectors);

    std::vector<DecoderGraph::Node> suffix_nodes(2);
    for (auto swit = suffix_subwords.begin(); swit != suffix_subwords.end(); ++swit) {
        if (swit->find("<") != string::npos) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(*swit, subword_triphones);
        // One phone subwords not connected yet to the main tree
        if (num_triphones(subword_triphones) < 2) continue;
        vector<DecoderGraph::Node> subword_nodes;
        triphones_to_state_chain(subword_triphones, subword_nodes);
        subword_nodes[3].from_fanin.insert(m_lexicon[*swit][0]);
        subword_nodes[subword_nodes.size()-4].to_fanout.insert(m_lexicon[*swit].back());
        add_nodes_to_tree(suffix_nodes, subword_nodes);
    }
    lookahead_to_arcs(suffix_nodes);

    vector<pair<unsigned int, string> > suffix_fanout_connectors, suffix_fanin_connectors;
    collect_crossword_connectors(suffix_nodes, suffix_fanout_connectors, suffix_fanin_connectors);

    offset(suffix_nodes, prefix_nodes.size());
    offset(suffix_fanout_connectors, prefix_nodes.size());
    offset(suffix_fanin_connectors, prefix_nodes.size());
    prefix_nodes[END_NODE].arcs.insert(START_NODE);
    suffix_nodes[END_NODE].arcs.insert(START_NODE);

    prefix_nodes.insert(prefix_nodes.end(), suffix_nodes.begin(), suffix_nodes.end());

    // Prefix-suffix cross-unit network
    map<string, int> fanout;
    map<string, int> fanin;
    vector<DecoderGraph::Node> cw_nodes;
    create_crossword_network(prefix_fanout_connectors, suffix_fanin_connectors, cw_nodes, fanout, fanin);
    minimize_crossword_network(cw_nodes, fanout, fanin);
    connect_crossword_network(prefix_nodes,
                              prefix_fanout_connectors, suffix_fanin_connectors,
                              cw_nodes, fanout, fanin, false);

    // Suffix-suffix cross-unit network
    fanout.clear();
    fanin.clear();
    cw_nodes.clear();
    create_crossword_network(suffix_fanout_connectors, suffix_fanin_connectors, cw_nodes, fanout, fanin);
    minimize_crossword_network(cw_nodes, fanout, fanin);
    connect_crossword_network(prefix_nodes,
                              suffix_fanout_connectors, suffix_fanin_connectors,
                              cw_nodes, fanout, fanin, false);

    m_nodes.swap(prefix_nodes);
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


void
NoWBSubwordGraph::offset(vector<pair<unsigned int, string> > &connectors,
                         int offset)
{
    for (auto cit = connectors.begin(); cit != connectors.end(); ++cit)
        cit->first += offset;
}


void
NoWBSubwordGraph::collect_crossword_connectors(vector<DecoderGraph::Node> &nodes,
                                               vector<pair<unsigned int, string> > &fanout_connectors,
                                               vector<pair<unsigned int, string> > &fanin_connectors)
{
    fanout_connectors.clear();
    fanin_connectors.clear();
    for (unsigned int i=0; i<nodes.size(); i++) {
        Node &nd = nodes[i];
        for (auto fiit = nd.from_fanin.begin(); fiit != nd.from_fanin.end(); ++fiit)
            fanin_connectors.push_back(make_pair(i, *fiit));
        for (auto foit = nd.to_fanout.begin(); foit != nd.to_fanout.end(); ++foit)
            fanout_connectors.push_back(make_pair(i, *foit));
    }
}

