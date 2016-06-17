#include <cassert>

#include "RWBSubwordGraph.hh"

using namespace std;


RWBSubwordGraph::RWBSubwordGraph()
{
}


RWBSubwordGraph::RWBSubwordGraph(const set<string> &prefix_subwords,
                                   const set<string> &subwords,
                                   bool verbose)
{
    create_graph(prefix_subwords, subwords, verbose);
}


void
RWBSubwordGraph::create_crossunit_network(vector<pair<unsigned int, string> > &fanout_triphones,
                                           vector<pair<unsigned int, string> > &fanin_triphones,
                                           set<string> &one_phone_prefix_subwords,
                                           set<string> &one_phone_suffix_subwords,
                                           vector<DecoderGraph::Node> &nodes,
                                           map<string, int> &fanout,
                                           map<string, int> &fanin)
{
    int spp = m_states_per_phone;
    set<char> all_phones;
    set<char> suffix_phones;

    for (auto foit=fanout_triphones.begin(); foit != fanout_triphones.end(); ++foit)
        fanout[foit->second] = -1;

    for (auto fiit=fanin_triphones.begin(); fiit != fanin_triphones.end(); ++fiit)
        fanin[fiit->second] = -1;

    for (auto opswit = one_phone_prefix_subwords.begin(); opswit != one_phone_prefix_subwords.end(); ++opswit) {
        fanout[m_lexicon.at(*opswit)[0]] = -1;
        all_phones.insert(tphone(m_lexicon.at(*opswit)[0]));
    }

    for (auto opswit = one_phone_suffix_subwords.begin(); opswit != one_phone_suffix_subwords.end(); ++opswit) {
        fanin[m_lexicon.at(*opswit)[0]] = -1;
        suffix_phones.insert(tphone(m_lexicon.at(*opswit)[0]));
        all_phones.insert(tphone(m_lexicon.at(*opswit)[0]));
    }

    // Fanout last triphone + phone from one phone subwords, all combinations to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {
        if (tlc(foit->first) == SIL_CTXT) continue;
        for (auto phit = all_phones.begin(); phit != all_phones.end(); ++phit) {
            string fanoutt = construct_triphone(tphone(foit->first), *phit, SIL_CTXT);
            fanout[fanoutt] = -1;
        }
    }

    // All suffix one phone combinations to fanout
    for (auto fphit = suffix_phones.begin(); fphit != suffix_phones.end(); ++fphit)
        for (auto sphit = suffix_phones.begin(); sphit != suffix_phones.end(); ++sphit) {
            string fanoutt = construct_triphone(*fphit, *sphit, SIL_CTXT);
            fanout[fanoutt] = -1;
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

            char single_phone = tphone(m_lexicon[*opswit][0]);
            string triphone = construct_triphone(tlc(foit->first), tphone(foit->first), single_phone);
            string fanout_loop_connector = construct_triphone(tphone(foit->first), single_phone, SIL_CTXT);

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

        if (tlc(foit->first) == SIL_CTXT) continue;

        for (auto opswit = one_phone_prefix_subwords.begin(); opswit != one_phone_prefix_subwords.end(); ++opswit) {

            char single_phone = tphone(m_lexicon[*opswit][0]);
            string triphone = construct_triphone(tlc(foit->first), tphone(foit->first), single_phone);
            string fanout_loop_connector = construct_triphone(tphone(foit->first), single_phone, SIL_CTXT);

            if (fanout.find(fanout_loop_connector) == fanout.end()) {
                cerr << "problem in connecting fanout loop for one phone subword: " << *opswit << endl;
                cerr << fanout_loop_connector << endl;
                assert(false);
            }

            int idx = connect_triphone(nodes, triphone, foit->second);
            idx = connect_triphone(nodes, SHORT_SIL, idx);
            idx = connect_word(nodes, *opswit, idx);
            nodes[idx].arcs.insert(fanout[fanout_loop_connector]);
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        cwnit->flags |= NODE_CW;
}


void
RWBSubwordGraph::create_crossword_network(vector<pair<unsigned int, string> > &fanout_triphones,
                                           vector<pair<unsigned int, string> > &fanin_triphones,
                                           set<string> &one_phone_prefix_subwords,
                                           set<string> &one_phone_suffix_subwords,
                                           vector<DecoderGraph::Node> &nodes,
                                           map<string, int> &fanout,
                                           map<string, int> &fanin)
{
    int spp = m_states_per_phone;
    set<char> phones;

    for (auto foit=fanout_triphones.begin(); foit != fanout_triphones.end(); ++foit)
        fanout[foit->second] = -1;

    for (auto fiit=fanin_triphones.begin(); fiit != fanin_triphones.end(); ++fiit)
        fanin[fiit->second] = -1;

    for (auto opswit = one_phone_prefix_subwords.begin(); opswit != one_phone_prefix_subwords.end(); ++opswit)
        fanout[m_lexicon.at(*opswit)[0]] = -1;

    for (auto opswit = one_phone_suffix_subwords.begin(); opswit != one_phone_suffix_subwords.end(); ++opswit) {
        fanin[m_lexicon.at(*opswit)[0]] = -1;
        phones.insert(tphone(m_lexicon.at(*opswit)[0]));
    }

    // All phone-phone combinations from one phone subwords to fanout
    for (auto fphit = phones.begin(); fphit != phones.end(); ++fphit)
        for (auto sphit = phones.begin(); sphit != phones.end(); ++sphit) {
            string fanoutt = DecoderGraph::construct_triphone(*fphit, *sphit, SIL_CTXT);
            fanout[fanoutt] = -1;
        }

    // Fanout last triphone + phone from one phone subwords, all combinations to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {
        if (tlc(foit->first) == SIL_CTXT) continue;
        for (auto phit = phones.begin(); phit != phones.end(); ++phit) {
            string fanoutt = DecoderGraph::construct_triphone(tphone(foit->first), *phit, SIL_CTXT);
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

    // Add loops for one phone suffix subwords from fanout back to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        for (auto opswit = one_phone_suffix_subwords.begin(); opswit != one_phone_suffix_subwords.end(); ++opswit) {

            char single_phone = tphone(m_lexicon[*opswit][0]);
            string triphone = construct_triphone(tlc(foit->first), tphone(foit->first), single_phone);
            string fanout_loop_connector = construct_triphone(tphone(foit->first), single_phone, SIL_CTXT);

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
RWBSubwordGraph::connect_crossword_network(vector<DecoderGraph::Node> &nodes,
                                            vector<pair<unsigned int, string> > &fanout_connectors,
                                            vector<pair<unsigned int, string> > &fanin_connectors,
                                            vector<DecoderGraph::Node> &cw_nodes,
                                            map<string, int> &fanout,
                                            map<string, int> &fanin)
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

    for (auto focit = fanout_connectors.begin(); focit != fanout_connectors.end(); ++focit) {
        DecoderGraph::Node &nd = nodes[focit->first];
        nd.arcs.insert(fanout[focit->second]);
    }

}


void
RWBSubwordGraph::connect_one_phone_subwords_from_start_to_cw(const set<string> &subwords,
                                                              vector<DecoderGraph::Node> &nodes,
                                                              map<string, int> &fanout)
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        vector<string> &triphones = m_lexicon[*swit];
        if (triphones.size() != 1 || !is_triphone(triphones[0])) continue;
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
RWBSubwordGraph::connect_one_phone_subwords_from_cw_to_end(const set<string> &subwords,
                                                            vector<DecoderGraph::Node> &nodes,
                                                            map<string, int> &fanin)
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        vector<string> &triphones = m_lexicon[*swit];
        if (triphones.size() != 1 || !is_triphone(triphones[0])) continue;
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
RWBSubwordGraph::create_graph(const set<string> &prefix_subwords,
                               const set<string> &suffix_subwords,
                               bool verbose)
{
    set<string> one_phone_prefix_subwords;
    get_one_phone_subwords(prefix_subwords, one_phone_prefix_subwords);
    set<string> one_phone_suffix_subwords;
    get_one_phone_subwords(suffix_subwords, one_phone_suffix_subwords);

    // Construct prefix/stem tree
    vector<DecoderGraph::Node> prefix_nodes(2);
    for (auto swit = prefix_subwords.begin(); swit != prefix_subwords.end(); ++swit) {
        if (swit->find("<") != string::npos) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(*swit, subword_triphones);
        // One phone subwords not connected to the main tree
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

    // Construct suffix tree
    std::vector<DecoderGraph::Node> suffix_nodes(2);
    for (auto swit = suffix_subwords.begin(); swit != suffix_subwords.end(); ++swit) {
        if (swit->find("<") != string::npos) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(*swit, subword_triphones);
        // One phone subwords not connected to the main tree
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
    offset(prefix_nodes, suffix_nodes.size());
    offset(prefix_fanout_connectors, suffix_nodes.size());
    offset(prefix_fanin_connectors, suffix_nodes.size());
    suffix_nodes[END_NODE].arcs.insert(START_NODE);
    suffix_nodes.insert(suffix_nodes.end(), prefix_nodes.begin(), prefix_nodes.end());
    prefix_nodes.clear();

    vector<DecoderGraph::Node> nodes;
    nodes.swap(suffix_nodes); suffix_nodes.clear();
    set_reverse_arcs_also_from_unreachable(nodes);
    merge_nodes(nodes, START_NODE, prefix_size+START_NODE);
    clear_reverse_arcs(nodes);

    // Cross-unit network (prefix-suffix, suffix-suffix)
    /*
    if (verbose) cerr << "creating cross-unit network" << endl;
    map<string, int> cu_fanout;
    map<string, int> cu_fanin;
    vector<DecoderGraph::Node> cu_nodes;
    vector<pair<unsigned int, string> > all_fanout_connectors = suffix_fanout_connectors;
    all_fanout_connectors.insert(all_fanout_connectors.end(),
                                 prefix_fanout_connectors.begin(),
                                 prefix_fanout_connectors.end());
    create_crossunit_network(all_fanout_connectors, suffix_fanin_connectors,
                             one_phone_prefix_subwords, one_phone_suffix_subwords,
                             cu_nodes, cu_fanout, cu_fanin);
    minimize_crossword_network(cu_nodes, cu_fanout, cu_fanin);
    if (verbose) cerr << "tied cross-unit network size: " << cu_nodes.size() << endl;
    connect_crossword_network(nodes,
                              all_fanout_connectors, suffix_fanin_connectors,
                              cu_nodes, cu_fanout, cu_fanin);

    connect_one_phone_subwords_from_start_to_cw(one_phone_prefix_subwords, nodes, cu_fanout);
    connect_one_phone_subwords_from_cw_to_end(one_phone_suffix_subwords, nodes, cu_fanin);
    */

    // Cross-word network
    /*
    if (verbose) cerr << "creating cross-word network" << endl;
    map<string, int> cw_fanout;
    map<string, int> cw_fanin;
    vector<DecoderGraph::Node> cw_nodes;
    create_crossword_network(all_fanout_connectors, prefix_fanin_connectors,
                             one_phone_prefix_subwords, one_phone_suffix_subwords,
                             cw_nodes, cw_fanout, cw_fanin);
    minimize_crossword_network(cw_nodes, cw_fanout, cw_fanin);
    if (verbose) cerr << "tied cross-word network size: " << cw_nodes.size() << endl;
    connect_crossword_network(nodes,
                              all_fanout_connectors, prefix_fanin_connectors,
                              cw_nodes, cw_fanout, cw_fanin);

    connect_one_phone_subwords_from_start_to_cw(one_phone_prefix_subwords, nodes, cw_fanout);
    connect_one_phone_subwords_from_cw_to_end(one_phone_suffix_subwords, nodes, cw_fanin);
    */

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
RWBSubwordGraph::offset(vector<DecoderGraph::Node> &nodes,
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
RWBSubwordGraph::get_one_phone_subwords(const set<string> &subwords,
                                         set<string> &one_phone_subwords) const
{
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit)
        if (m_lexicon.at(*swit).size() == 1)
            one_phone_subwords.insert(*swit);
}


void
RWBSubwordGraph::offset(vector<pair<unsigned int, string> > &connectors,
                         int offset)
{
    for (auto cit = connectors.begin(); cit != connectors.end(); ++cit)
        cit->first += offset;
}


void
RWBSubwordGraph::collect_crossword_connectors(vector<DecoderGraph::Node> &nodes,
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


void
RWBSubwordGraph::create_forced_path(vector<DecoderGraph::Node> &nodes,
                                     vector<string> &sentence,
                                     map<int, string> &node_labels)
{
    vector<string> word;
    vector<TriphoneNode> tnodes;

    // Create initial triphone graph
    tnodes.push_back(TriphoneNode(-1, m_hmm_map["__"]));

    for (int i=1; i<(int)sentence.size(); i++) {
        if (i > 2 && (sentence[i] == "</s>" || sentence[i][0] == '_' || sentence[i][0] == '#')) {
            vector<TriphoneNode> word_tnodes;
            triphonize(word, word_tnodes);
            tnodes.insert(tnodes.end(), word_tnodes.begin(), word_tnodes.end());
            tnodes.push_back(TriphoneNode(-1, m_hmm_map["__"]));
            word.clear();
            if (sentence[i][0] == '_' || sentence[i][0] == '#')
                word.push_back(sentence[i]);
        }
        else (word.push_back(sentence[i]));
    }

    // Convert to HMM states, also with crossword context
    nodes.clear(); nodes.resize(1);
    int idx = 0, crossword_start = -1;
    string crossword_left, crossword_right, label;
    node_labels.clear();

    for (int t=0; t<(int)tnodes.size(); t++)
        if (tnodes[t].hmm_id != -1) {

            if (m_hmms[tnodes[t].hmm_id].label.length() == 5 &&
                m_hmms[tnodes[t].hmm_id].label[4] == '_')
            {
                crossword_start = idx;
                crossword_left = m_hmms[tnodes[t].hmm_id].label;
            }

            idx = connect_triphone(nodes, tnodes[t].hmm_id, idx, node_labels);

            if (crossword_start != -1 &&
                m_hmms[tnodes[t].hmm_id].label.length() == 5 &&
                m_hmms[tnodes[t].hmm_id].label[0] == '_')
            {
                idx = connect_dummy(nodes, idx);

                crossword_right = m_hmms[tnodes[t].hmm_id].label;
                crossword_left[4] = crossword_right[2];
                crossword_right[0] = crossword_left[2];

                int tmp = connect_triphone(nodes, crossword_left, crossword_start, node_labels);
                tmp = connect_triphone(nodes, "_", tmp, node_labels);
                tmp = connect_triphone(nodes, crossword_right, tmp, node_labels);

                nodes[tmp].arcs.insert(idx);
            }

        }
        else
            idx = connect_word(nodes, tnodes[t].subword_id, idx);
}

