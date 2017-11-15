#include <algorithm>
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
LRWBSubwordGraph::create_crossunit_network(
        vector<pair<unsigned int, string> > &fanout_triphones,
        vector<pair<unsigned int, string> > &fanin_triphones,
        vector<pair<unsigned int, string> > &cw_fanout_triphones,
        set<string> &one_phone_prefix_subwords,
        set<string> &one_phone_stem_subwords,
        set<string> &one_phone_suffix_subwords,
        vector<DecoderGraph::Node> &nodes,
        map<string, int> &fanout,
        map<string, int> &fanin)
{
    int spp = m_states_per_phone;
    set<char> prefix_phones, stem_phones, suffix_phones;

    for (auto foit=fanout_triphones.begin(); foit != fanout_triphones.end(); ++foit)
        fanout[foit->second] = -1;

    for (auto fiit=fanin_triphones.begin(); fiit != fanin_triphones.end(); ++fiit)
        fanin[fiit->second] = -1;

    for (auto opswit = one_phone_prefix_subwords.begin(); opswit != one_phone_prefix_subwords.end(); ++opswit) {
        fanout[m_lexicon.at(*opswit)[0]] = -1;
        prefix_phones.insert(tphone(m_lexicon.at(*opswit)[0]));
    }

    for (auto opswit = one_phone_stem_subwords.begin(); opswit != one_phone_stem_subwords.end(); ++opswit)
        stem_phones.insert(tphone(m_lexicon.at(*opswit)[0]));

    for (auto opswit = one_phone_suffix_subwords.begin(); opswit != one_phone_suffix_subwords.end(); ++opswit) {
        fanin[m_lexicon.at(*opswit)[0]] = -1;
        suffix_phones.insert(tphone(m_lexicon.at(*opswit)[0]));
    }

    // Fanout last triphone + phone from stem one phone subwords, all combinations to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {
        if (tlc(foit->first) == SIL_CTXT) continue;
        for (auto phit = stem_phones.begin(); phit != stem_phones.end(); ++phit) {
            string fanoutt = construct_triphone(tphone(foit->first), *phit, SIL_CTXT);
            fanout[fanoutt] = -1;
        }
    }

    // All prefix + stem one phone combinations to fanout
    for (auto fphit = prefix_phones.begin(); fphit != prefix_phones.end(); ++fphit)
        for (auto sphit = stem_phones.begin(); sphit != stem_phones.end(); ++sphit) {
            string fanoutt = construct_triphone(*fphit, *sphit, SIL_CTXT);
            fanout[fanoutt] = -1;
        }

    // Ending words may be connected by a one phone prefix to the cross-unit network
    for (auto cwit = cw_fanout_triphones.begin(); cwit != cw_fanout_triphones.end(); ++cwit) {
        for (auto ppit = prefix_phones.begin(); ppit != prefix_phones.end(); ++ppit) {
            string fanoutt = construct_triphone(tphone(cwit->second), *ppit, SIL_CTXT);
            fanout[fanoutt] = -1;
        }
    }

    for (auto fphit = suffix_phones.begin(); fphit != suffix_phones.end(); ++fphit) {
        for (auto sphit = prefix_phones.begin(); sphit != prefix_phones.end(); ++sphit) {
            string fanint = construct_triphone(SIL_CTXT, *fphit, *sphit);
            fanin[fanint] = -1;
            string fanoutt = construct_triphone(*fphit, *sphit, SIL_CTXT);
            fanout[fanoutt] = -1;
        }
    }

    // Construct the main cross-unit network
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

    // Add loops for one phone stem subwords from fanout back to fanout
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {
        for (auto opswit = one_phone_stem_subwords.begin(); opswit != one_phone_stem_subwords.end(); ++opswit) {
            char single_phone = tphone(m_lexicon[*opswit][0]);
            string triphone = construct_triphone(tlc(foit->first), tphone(foit->first), single_phone);
            string fanout_loop_connector = construct_triphone(tphone(foit->first), single_phone, SIL_CTXT);

            if (fanout.find(fanout_loop_connector) == fanout.end()) {
                cerr << "problem in connecting cross-unit fanout loop for one phone subword: " << *opswit << endl;
                cerr << fanout_loop_connector << endl;
                assert(false);
            }

            int idx = connect_triphone(nodes, triphone, foit->second);
            idx = connect_word(nodes, *opswit, idx);
            nodes[idx].arcs.insert(fanout[fanout_loop_connector]);
        }
    }

    // Add loops for one phone suffix+prefix subwords from fanin to fanout
    for (auto sswit = one_phone_suffix_subwords.begin(); sswit != one_phone_suffix_subwords.end(); ++sswit) {
        for (auto pswit = one_phone_prefix_subwords.begin(); pswit != one_phone_prefix_subwords.end(); ++pswit) {
            char suffix_phone = tphone(m_lexicon[*sswit][0]);
            char prefix_phone = tphone(m_lexicon[*pswit][0]);
            string fanin_connector = construct_triphone(SIL_CTXT, suffix_phone, prefix_phone);
            string fanout_connector = construct_triphone(suffix_phone, prefix_phone, SIL_CTXT);
            if (fanin.find(fanin_connector) == fanin.end()
                    || fanout.find(fanout_connector) == fanout.end())
            {
                cerr << "problem in connecting from fanin to fanout" << endl;
                cerr << fanin_connector << " " << fanout_connector << endl;
                assert(false);
            }

            int idx = connect_word(nodes, *sswit, fanin[fanin_connector]);
            idx = connect_triphone(nodes, SHORT_SIL, idx);
            idx = connect_word(nodes, *pswit, idx);
            nodes[idx].arcs.insert(fanout[fanout_connector]);
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        cwnit->flags |= NODE_CW;
}


void
LRWBSubwordGraph::create_crossword_network(vector<pair<unsigned int, string> > &fanout_triphones,
        vector<pair<unsigned int, string> > &fanin_triphones,
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

    for (auto opswit = one_phone_suffix_subwords.begin(); opswit != one_phone_suffix_subwords.end(); ++opswit) {
        fanin[m_lexicon.at(*opswit)[0]] = -1;
        all_phones.insert(tphone(m_lexicon.at(*opswit)[0]));
        suffix_phones.insert(tphone(m_lexicon.at(*opswit)[0]));
    }

    // Construct the main cross-word network
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
LRWBSubwordGraph::connect_crossword_network(vector<DecoderGraph::Node> &nodes,
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
LRWBSubwordGraph::connect_one_phone_subwords_from_start_to_cw(const set<string> &subwords,
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
LRWBSubwordGraph::connect_one_phone_subwords_from_cw_to_end(const set<string> &subwords,
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
LRWBSubwordGraph::connect_one_phone_subwords_from_fanout_to_fanin(const set<string> &subwords,
        vector<DecoderGraph::Node> &nodes,
        map<string, int> &fanout,
        map<string, int> &fanin)
{
    for (auto fofit = fanout.begin(); fofit != fanout.end(); ++fofit) {
        for (auto fifit = fanin.begin(); fifit != fanin.end(); ++fifit) {
            for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
                char phone1, phone2, phone3, phone4, phone5;
                phone1 = fofit->first[0];
                phone2 = fofit->first[2];
                phone3 = m_lexicon.at(*swit)[0][2];
                phone4 = fifit->first[2];
                phone5 = fifit->first[4];
                string triphone1 = construct_triphone(phone1,phone2,phone3);
                string triphone2 = construct_triphone(phone2,phone3,phone4);
                string triphone3 = construct_triphone(phone3,phone4,phone5);
                int idx = connect_triphone(nodes, triphone1, fofit->second);
                idx = connect_triphone(nodes, triphone2, idx);
                idx = connect_word(nodes, *swit, idx);
                idx = connect_triphone(nodes, SHORT_SIL, idx);
                idx = connect_triphone(nodes, triphone3, idx);
                nodes[idx].arcs.insert(fifit->second);
            }
        }
    }
}


void
LRWBSubwordGraph::connect_one_phone_prefix_subwords_to_fanout(
    const set<string> &one_phone_prefix_subwords,
    vector<DecoderGraph::Node> &nodes,
    vector<pair<unsigned int, string> > &fanout_connectors,
    map<string, int> &fanout)
{
    for (auto focit = fanout_connectors.begin(); focit != fanout_connectors.end(); ++focit) {
        for (auto swit = one_phone_prefix_subwords.begin(); swit != one_phone_prefix_subwords.end(); ++swit) {
            string triphone = focit->second;
            triphone[4] = tphone(m_lexicon[*swit][0]);
            string target_fanout = construct_triphone(tphone(triphone), trc(triphone), SIL_CTXT);

            int idx = connect_triphone(nodes, triphone, focit->first);
            idx = connect_triphone(nodes, SHORT_SIL, idx);
            idx = connect_word(nodes, *swit, idx);
            nodes[idx].arcs.insert(fanout[target_fanout]);
        }
    }
}


void
LRWBSubwordGraph::get_one_phone_prefix_subwords(const set<string> &prefix_subwords,
        set<string> &one_phone_prefix_subwords)
{
    for (auto swit = prefix_subwords.begin(); swit != prefix_subwords.end(); ++swit)
        if (m_lexicon.at(*swit).size() == 1)
            one_phone_prefix_subwords.insert(*swit);
}


void
LRWBSubwordGraph::get_one_phone_stem_subwords(const set<string> &stem_subwords,
        set<string> &one_phone_stem_subwords)
{
    for (auto swit = stem_subwords.begin(); swit != stem_subwords.end(); ++swit)
        if (m_lexicon.at(*swit).size() == 1)
            one_phone_stem_subwords.insert(*swit);
}


void
LRWBSubwordGraph::get_one_phone_suffix_subwords(const set<string> &suffix_subwords,
        set<string> &one_phone_suffix_subwords)
{
    for (auto swit = suffix_subwords.begin(); swit != suffix_subwords.end(); ++swit)
        if (m_lexicon.at(*swit).size() == 1)
            one_phone_suffix_subwords.insert(*swit);
}


void
LRWBSubwordGraph::offset(vector<DecoderGraph::Node> &nodes,
                         int offset)
{
    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit) {
        set<unsigned int> arcs;
        for (auto ait = nit->arcs.begin(); ait != nit->arcs.end(); ++ait)
            arcs.insert(*ait + offset);
        nit->arcs.swap(arcs);

        set<unsigned int> rev_arcs;
        for (auto ait = nit->reverse_arcs.begin(); ait != nit->reverse_arcs.end(); ++ait)
            rev_arcs.insert(*ait + offset);
        nit->reverse_arcs.swap(rev_arcs);
    }
}


void
LRWBSubwordGraph::offset(vector<pair<unsigned int, string> > &connectors,
                         int offset)
{
    for (auto cit = connectors.begin(); cit != connectors.end(); ++cit)
        cit->first += offset;
}


void
LRWBSubwordGraph::create_graph(const set<string> &prefix_subwords,
                               const set<string> &stem_subwords,
                               const set<string> &suffix_subwords,
                               const set<string> &word_subwords,
                               bool verbose)
{
    // Construct tree for subwords starting new words
    vector<DecoderGraph::Node> prefix_and_word_nodes(2);
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
        subword_nodes.resize(subword_nodes.size()-3);
        add_nodes_to_tree(prefix_and_word_nodes, subword_nodes, false);
    }
    for (auto swit = word_subwords.begin(); swit != word_subwords.end(); ++swit) {
        if (swit->find("<") != string::npos) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(*swit, subword_triphones);
        // One phone subwords not connected to the main tree
        if (num_triphones(subword_triphones) < 2) continue;
        vector<DecoderGraph::Node> subword_nodes;
        triphones_to_state_chain(subword_triphones, subword_nodes);
        subword_nodes[3].from_fanin.insert(m_lexicon[*swit][0]);
        subword_nodes[subword_nodes.size()-4].to_fanout_2.insert(m_lexicon[*swit].back());
        add_nodes_to_tree(prefix_and_word_nodes, subword_nodes);
    }
    if (prefix_and_word_nodes.size() == 2) {
        cerr << "Warning, no subwords in the word initial tree" << endl;
        //exit(1);
    }
    lookahead_to_arcs(prefix_and_word_nodes);

    // Collect fan-in connectors for the word initial tree
    vector<pair<unsigned int, string> > wi_fanin_connectors;
    for (unsigned int i=0; i<prefix_and_word_nodes.size(); i++) {
        Node &nd = prefix_and_word_nodes[i];
        for (auto fiit = nd.from_fanin.begin(); fiit != nd.from_fanin.end(); ++fiit)
            wi_fanin_connectors.push_back(make_pair(i, *fiit));
    }

    // Construct tree for subwords continuing words
    vector<DecoderGraph::Node> stem_and_suffix_nodes(2);
    for (auto swit = stem_subwords.begin(); swit != stem_subwords.end(); ++swit) {
        if (swit->find("<") != string::npos) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(*swit, subword_triphones);
        // One phone subwords not connected to the main tree
        if (num_triphones(subword_triphones) < 2) continue;
        vector<DecoderGraph::Node> subword_nodes;
        triphones_to_state_chain(subword_triphones, subword_nodes);
        subword_nodes[3].from_fanin.insert(m_lexicon[*swit][0]);
        subword_nodes[subword_nodes.size()-4].to_fanout.insert(m_lexicon[*swit].back());
        subword_nodes.resize(subword_nodes.size()-3);
        add_nodes_to_tree(stem_and_suffix_nodes, subword_nodes, false);
    }
    for (auto swit = suffix_subwords.begin(); swit != suffix_subwords.end(); ++swit) {
        if (swit->find("<") != string::npos) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(*swit, subword_triphones);
        // One phone subwords not connected to the main tree
        if (num_triphones(subword_triphones) < 2) continue;
        vector<DecoderGraph::Node> subword_nodes;
        triphones_to_state_chain(subword_triphones, subword_nodes);
        subword_nodes[3].from_fanin.insert(m_lexicon[*swit][0]);
        subword_nodes[subword_nodes.size()-4].to_fanout_2.insert(m_lexicon[*swit].back());
        add_nodes_to_tree(stem_and_suffix_nodes, subword_nodes);
    }
    if (stem_and_suffix_nodes.size() == 2) {
        cerr << "Warning, no subwords in the stem and suffix tree" << endl;
        //exit(1);
    }
    lookahead_to_arcs(stem_and_suffix_nodes);

    // Collect fan-in connectors for the word-continuing tree
    vector<pair<unsigned int, string> > wc_fanin_connectors;
    for (unsigned int i=0; i<stem_and_suffix_nodes.size(); i++) {
        Node &nd = stem_and_suffix_nodes[i];
        for (auto fiit = nd.from_fanin.begin(); fiit != nd.from_fanin.end(); ++fiit)
            wc_fanin_connectors.push_back(make_pair(i, *fiit));
    }

    // Combine word initial and stem/suffix trees
    if (verbose) cerr << "adding trees to the same graph" << endl;
    int stem_suffix_size = stem_and_suffix_nodes.size();
    offset(prefix_and_word_nodes, stem_suffix_size);
    offset(wi_fanin_connectors, stem_suffix_size);
    stem_and_suffix_nodes.insert(stem_and_suffix_nodes.end(), prefix_and_word_nodes.begin(), prefix_and_word_nodes.end());
    prefix_and_word_nodes.clear();

    vector<DecoderGraph::Node> nodes;
    nodes.swap(stem_and_suffix_nodes);
    stem_and_suffix_nodes.clear();
    set_reverse_arcs_also_from_unreachable(nodes);
    merge_nodes(nodes, START_NODE, stem_suffix_size+START_NODE);
    merge_nodes(nodes, END_NODE, stem_suffix_size+END_NODE);
    clear_reverse_arcs(nodes);
    connect_end_to_start_node(nodes);

    // Collect fan-out connectors for the cross-unit network
    vector<pair<unsigned int, string> > cu_fanout_connectors;
    for (unsigned int i=0; i<nodes.size(); i++) {
        Node &nd = nodes[i];
        for (auto foit = nd.to_fanout.begin(); foit != nd.to_fanout.end(); ++foit)
            cu_fanout_connectors.push_back(make_pair(i, *foit));
    }

    // Collect fan-out connectors for the cross-word network
    vector<pair<unsigned int, string> > cw_fanout_connectors;
    for (unsigned int i=0; i<nodes.size(); i++) {
        Node &nd = nodes[i];
        for (auto foit = nd.to_fanout_2.begin(); foit != nd.to_fanout_2.end(); ++foit)
            cw_fanout_connectors.push_back(make_pair(i, *foit));
    }

    set<string> one_phone_prefix_subwords,
        one_phone_stem_subwords, one_phone_suffix_subwords;
    get_one_phone_prefix_subwords(prefix_subwords, one_phone_prefix_subwords);
    get_one_phone_stem_subwords(stem_subwords, one_phone_stem_subwords);
    get_one_phone_suffix_subwords(suffix_subwords, one_phone_suffix_subwords);

    // Cross-unit network
    if (verbose) cerr << "creating cross-unit network" << endl;
    set<string> dummy_one_phones;
    map<string, int> cu_fanout;
    map<string, int> cu_fanin;
    vector<DecoderGraph::Node> cu_nodes;
    create_crossunit_network(cu_fanout_connectors,
                             wc_fanin_connectors,
                             cw_fanout_connectors,
                             one_phone_prefix_subwords,
                             one_phone_stem_subwords,
                             one_phone_suffix_subwords,
                             cu_nodes, cu_fanout, cu_fanin);
    minimize_crossword_network(cu_nodes, cu_fanout, cu_fanin);
    if (verbose) cerr << "tied cross-unit network size: " << cu_nodes.size() << endl;
    connect_crossword_network(nodes,
                              cu_fanout_connectors, wc_fanin_connectors,
                              cu_nodes, cu_fanout, cu_fanin);

    connect_one_phone_subwords_from_start_to_cw(one_phone_prefix_subwords, nodes, cu_fanout);
    connect_one_phone_subwords_from_cw_to_end(one_phone_suffix_subwords, nodes, cu_fanin);

    // Cross-word network
    if (verbose) cerr << "creating cross-word network" << endl;
    map<string, int> cw_fanout;
    map<string, int> cw_fanin;
    vector<DecoderGraph::Node> cw_nodes;
    create_crossword_network(cw_fanout_connectors, wi_fanin_connectors,
                             one_phone_suffix_subwords,
                             cw_nodes, cw_fanout, cw_fanin);
    minimize_crossword_network(cw_nodes, cw_fanout, cw_fanin);
    if (verbose) cerr << "tied cross-word network size: " << cw_nodes.size() << endl;
    connect_crossword_network(nodes,
                              cw_fanout_connectors, wi_fanin_connectors,
                              cw_nodes, cw_fanout, cw_fanin);

    connect_one_phone_subwords_from_fanout_to_fanin(one_phone_suffix_subwords,
            nodes,
            cu_fanout,
            cw_fanin);

    connect_one_phone_prefix_subwords_to_fanout(one_phone_prefix_subwords,
            nodes,
            cw_fanout_connectors,
            cu_fanout);

    m_nodes.swap(nodes);

    if (verbose) cerr << "Removing cw dummies.." << endl;
    remove_cw_dummies(m_nodes);
    if (verbose) cerr << "Removing nodes with no arcs.." << endl;
    remove_nodes_with_no_arcs(m_nodes);

    prune_unreachable_nodes(m_nodes);

    if (verbose) cerr << "number of nodes: " << reachable_graph_nodes(m_nodes) << endl;
}

