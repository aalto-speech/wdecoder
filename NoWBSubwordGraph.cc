#include <cassert>

#include "NoWBSubwordGraph.hh"

using namespace std;


NoWBSubwordGraph::NoWBSubwordGraph()
{
}


NoWBSubwordGraph::NoWBSubwordGraph(const set<string> &prefix_subwords,
                                   const set<string> &subwords,
                                   bool verbose)
{
    create_graph(prefix_subwords, subwords, verbose);
}


void
NoWBSubwordGraph::create_crossunit_network(vector<pair<unsigned int, string> > &fanout_triphones,
                                           vector<pair<unsigned int, string> > &fanin_triphones,
                                           set<string> &one_phone_prefix_subwords,
                                           set<string> &one_phone_suffix_subwords,
                                           vector<DecoderGraph::Node> &nodes,
                                           map<string, int> &fanout,
                                           map<string, int> &fanin,
                                           bool short_silence)
{
    int spp = m_states_per_phone;
    set<char> phones;

    for (auto foit=fanout_triphones.begin(); foit != fanout_triphones.end(); ++foit)
        fanout[foit->second] = -1;

    for (auto fiit=fanin_triphones.begin(); fiit != fanin_triphones.end(); ++fiit)
        fanin[fiit->second] = -1;

    for (auto opswit = one_phone_prefix_subwords.begin(); opswit != one_phone_prefix_subwords.end(); ++opswit) {
        fanout[m_lexicon.at(*opswit)[0]] = -1;
        phones.insert(m_lexicon.at(*opswit)[0][2]);
    }

    for (auto opswit = one_phone_suffix_subwords.begin(); opswit != one_phone_suffix_subwords.end(); ++opswit) {
        fanin[m_lexicon.at(*opswit)[0]] = -1;
        fanout[m_lexicon.at(*opswit)[0]] = -1;
        phones.insert(m_lexicon.at(*opswit)[0][2]);
    }

    // All phone-phone combinations from one phone subwords to fanout
    for (auto fphit = phones.begin(); fphit != phones.end(); ++fphit) {
        for (auto sphit = phones.begin(); sphit != phones.end(); ++sphit) {
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

            int idx = connect_triphone(nodes, triphone1, start_index);
            if (short_silence) idx = connect_triphone(nodes, "_", idx);

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


    // Add loops for one phone suffix subwords from fanout back to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        for (auto opswit = one_phone_suffix_subwords.begin(); opswit != one_phone_suffix_subwords.end(); ++opswit) {

            string single_phone = m_lexicon[*opswit][0];
            string triphone = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + string(1,single_phone[2]);
            string fanout_loop_connector = foit->first[2] + string(1,'-') + string(1,single_phone[2]) + string("+_");
            if (fanout.find(fanout_loop_connector) == fanout.end()) {
                cerr << "problem in connecting fanout loop for one phone subword: " << *opswit << endl;
                cerr << fanout_loop_connector << endl;
                assert(false);
            }

            int idx = connect_triphone(nodes, triphone, foit->second);
            idx = connect_word(nodes, *opswit, idx);
            nodes[idx].arcs.insert(fanout[fanout_loop_connector]);
        }
    }

    // Add loops for one phone prefix subwords from fanout back to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {
        for (auto opswit = one_phone_prefix_subwords.begin(); opswit != one_phone_prefix_subwords.end(); ++opswit) {

            string single_phone = m_lexicon[*opswit][0];
            string triphone = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + string(1,single_phone[2]);
            string fanout_loop_connector = foit->first[2] + string(1,'-') + string(1,single_phone[2]) + string("+_");
            if (fanout.find(fanout_loop_connector) == fanout.end()) {
                cerr << "problem in connecting fanout loop for one phone subword: " << *opswit << endl;
                cerr << fanout_loop_connector << endl;
                assert(false);
            }

            int idx = connect_triphone(nodes, triphone, foit->second);
            idx = connect_triphone(nodes, "_", idx);
            idx = connect_word(nodes, *opswit, idx);
            nodes[idx].arcs.insert(fanout[fanout_loop_connector]);
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        cwnit->flags |= NODE_CW;
}


void
NoWBSubwordGraph::create_crossword_network(vector<pair<unsigned int, string> > &fanout_triphones,
                                           vector<pair<unsigned int, string> > &fanin_triphones,
                                           set<string> &one_phone_prefix_subwords,
                                           set<string> &one_phone_suffix_subwords,
                                           vector<DecoderGraph::Node> &nodes,
                                           map<string, int> &fanout,
                                           map<string, int> &fanin,
                                           bool short_silence)
{
    int spp = m_states_per_phone;
    set<char> phones;

    for (auto foit=fanout_triphones.begin(); foit != fanout_triphones.end(); ++foit) {
        fanout[foit->second] = -1;
        phones.insert(foit->second[2]);
    }

    for (auto fiit=fanin_triphones.begin(); fiit != fanin_triphones.end(); ++fiit)
        fanin[fiit->second] = -1;

    for (auto opswit = one_phone_prefix_subwords.begin(); opswit != one_phone_prefix_subwords.end(); ++opswit) {
        fanout[m_lexicon.at(*opswit)[0]] = -1;
        phones.insert(m_lexicon.at(*opswit)[0][2]);
    }

    for (auto opswit = one_phone_suffix_subwords.begin(); opswit != one_phone_suffix_subwords.end(); ++opswit) {
        fanout[m_lexicon.at(*opswit)[0]] = -1;
        fanin[m_lexicon.at(*opswit)[0]] = -1;
        phones.insert(m_lexicon.at(*opswit)[0][2]);
    }

    // All phone-phone combinations from one phone subwords to fanout
    for (auto fphit = phones.begin(); fphit != phones.end(); ++fphit) {
        for (auto sphit = phones.begin(); sphit != phones.end(); ++sphit) {
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

            int idx = connect_triphone(nodes, triphone1, start_index);
            if (short_silence) idx = connect_triphone(nodes, "_", idx);

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

    // Add loops for one phone suffix subwords from fanout back to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        for (auto opswit = one_phone_suffix_subwords.begin(); opswit != one_phone_suffix_subwords.end(); ++opswit) {

            string single_phone = m_lexicon[*opswit][0];
            string triphone = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + string(1,single_phone[2]);
            string fanout_loop_connector = foit->first[2] + string(1,'-') + string(1,single_phone[2]) + string("+_");
            if (fanout.find(fanout_loop_connector) == fanout.end()) {
                cerr << "problem in connecting fanout loop for one phone subword: " << *opswit << endl;
                cerr << fanout_loop_connector << endl;
                assert(false);
            }

            int idx = connect_triphone(nodes, triphone, foit->second);
            idx = connect_word(nodes, *opswit, idx);
            nodes[idx].arcs.insert(fanout[fanout_loop_connector]);
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        cwnit->flags |= NODE_CW;
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

    for (auto ficit = fanin_connectors.begin(); ficit != fanin_connectors.end(); ++ficit)
        nodes[fanin[ficit->second]].arcs.insert(ficit->first);

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
    set<string> one_phone_prefix_subwords;
    get_one_phone_subwords(prefix_subwords, one_phone_prefix_subwords);
    set<string> one_phone_suffix_subwords;
    get_one_phone_subwords(suffix_subwords, one_phone_suffix_subwords);

    // Construct prefix tree
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
    if (verbose) cerr << "prefix tree size: " << reachable_graph_nodes(prefix_nodes) << endl;

    // Construct suffix/stem tree
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
    if (verbose) cerr << "stem/suffix tree size: " << reachable_graph_nodes(suffix_nodes) << endl;

    // Combine prefix and suffix/stem trees
    if (verbose) cerr << "combining trees" << endl;
    int prefix_size = prefix_nodes.size();
    offset(suffix_nodes, prefix_nodes.size());
    offset(suffix_fanout_connectors, prefix_nodes.size());
    offset(suffix_fanin_connectors, prefix_nodes.size());
    prefix_nodes[END_NODE].arcs.insert(START_NODE);
    suffix_nodes[END_NODE].arcs.insert(START_NODE);
    prefix_nodes.insert(prefix_nodes.end(), suffix_nodes.begin(), suffix_nodes.end());

    vector<DecoderGraph::Node> nodes;
    nodes.swap(prefix_nodes);
    merge_nodes(nodes, END_NODE, prefix_size+END_NODE);

    // Cross-unit network (prefix-suffix, suffix-suffix)
    if (verbose) cerr << "creating cross-unit network" << endl;
    map<string, int> fanout;
    map<string, int> fanin;
    vector<DecoderGraph::Node> cw_nodes;
    vector<pair<unsigned int, string> > all_fanout_connectors = suffix_fanout_connectors;
    all_fanout_connectors.insert(all_fanout_connectors.end(),
                                 prefix_fanout_connectors.begin(),
                                 prefix_fanout_connectors.end());
    create_crossunit_network(all_fanout_connectors, suffix_fanin_connectors,
                             one_phone_prefix_subwords, one_phone_suffix_subwords,
                             cw_nodes, fanout, fanin);
    minimize_crossword_network(cw_nodes, fanout, fanin);
    if (verbose) cerr << "tied cross-unit network size: " << cw_nodes.size() << endl;
    connect_crossword_network(nodes,
                              all_fanout_connectors, suffix_fanin_connectors,
                              cw_nodes, fanout, fanin, false);

    connect_one_phone_subwords_from_start_to_cw(one_phone_prefix_subwords, nodes, fanout);
    connect_one_phone_subwords_from_cw_to_end(one_phone_suffix_subwords, nodes, fanin);

    // Cross-word network
    if (verbose) cerr << "creating cross-word network" << endl;
    fanout.clear();
    fanin.clear();
    cw_nodes.clear();
    create_crossword_network(all_fanout_connectors, prefix_fanin_connectors,
                             one_phone_prefix_subwords, one_phone_suffix_subwords,
                             cw_nodes, fanout, fanin, true);
    minimize_crossword_network(cw_nodes, fanout, fanin);
    if (verbose) cerr << "tied cross-word network size: " << cw_nodes.size() << endl;
    connect_crossword_network(nodes,
                              all_fanout_connectors, prefix_fanin_connectors,
                              cw_nodes, fanout, fanin, false);

    connect_one_phone_subwords_from_start_to_cw(one_phone_prefix_subwords, nodes, fanout);
    connect_one_phone_subwords_from_cw_to_end(one_phone_suffix_subwords, nodes, fanin);

    m_nodes.swap(nodes);

    prune_unreachable_nodes(m_nodes);
    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;

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
NoWBSubwordGraph::get_one_phone_subwords(const set<string> &subwords,
                                         set<string> &one_phone_subwords) const
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit)
        if (m_lexicon.at(*swit).size() == 1)
            one_phone_subwords.insert(*swit);
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

