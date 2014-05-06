#include <cstdlib>
#include <sstream>

#include "defs.hh"
#include "gutils.hh"

using namespace std;

namespace gutils {

void triphonize(string word, vector<string> &triphones) {
    string tword = "_" + word + "_";
    triphones.clear();
    for (unsigned int i = 1; i < tword.length() - 1; i++) {
        stringstream tstring;
        tstring << tword[i - 1] << "-" << tword[i] << "+" << tword[i + 1];
        triphones.push_back(tstring.str());
    }
}

void triphonize(DecoderGraph &dg, string word, vector<string> &triphones) {
    if (dg.m_word_segs.find(word) != dg.m_word_segs.end()) {
        string tripstring;
        for (auto swit = dg.m_word_segs[word].begin();
                swit != dg.m_word_segs[word].end(); ++swit) {
            vector<string> &triphones = dg.m_lexicon[*swit];
            for (auto tit = triphones.begin(); tit != triphones.end(); ++tit)
                tripstring += (*tit)[2];
        }
        triphonize(tripstring, triphones);
    } else
        triphonize(word, triphones);
}

void triphonize(DecoderGraph &dg, vector<string> &word_seg,
        vector<DecoderGraph::TriphoneNode> &nodes) {
    nodes.clear();

    string tripstring;
    vector<pair<int, int> > word_id_positions;
    vector<string> triphones;
    for (auto swit = word_seg.begin(); swit != word_seg.end(); ++swit) {
        // FIXME
        vector<string> &triphones = dg.m_lexicon[*swit];
        for (auto tit = triphones.begin(); tit != triphones.end(); ++tit)
            tripstring += (*tit)[2];
        int word_id_pos = max(1, (int) (tripstring.size() - 1));
        word_id_positions.push_back(
                make_pair(dg.m_subword_map[*swit], word_id_pos));
    }
    triphonize(tripstring, triphones);

    for (unsigned int i = 1; i < word_id_positions.size(); i++) {
        word_id_positions[i].second += i;
        if (word_id_positions[i].second <= word_id_positions[i - 1].second)
            word_id_positions[i].second = word_id_positions[i - 1].second + 1;
    }

    for (auto triit = triphones.begin(); triit != triphones.end(); ++triit) {
        DecoderGraph::TriphoneNode trin;
        trin.hmm_id = dg.m_hmm_map[*triit];
        nodes.push_back(trin);
    }
    for (auto wit = word_id_positions.begin(); wit != word_id_positions.end();
            ++wit) {
        DecoderGraph::TriphoneNode trin;
        trin.subword_id = wit->first;
        nodes.insert(nodes.begin() + wit->second, trin);
    }
}

void triphonize_all_words(DecoderGraph &dg,
        map<string, vector<string> > &triphonized_words) {
    for (auto wit = dg.m_word_segs.begin(); wit != dg.m_word_segs.end();
            ++wit) {
        vector<string> triphones;
        triphonize(dg, wit->first, triphones);
        triphonized_words[wit->first] = triphones;
    }
}

void triphonize_subword(DecoderGraph &dg,
                        const string &subword,
                        vector<DecoderGraph::TriphoneNode> &nodes) {
    nodes.clear();

    string tripstring;

    vector<string> &triphones = dg.m_lexicon[subword];
    for (auto tit = triphones.begin(); tit != triphones.end(); ++tit)
        tripstring += (*tit)[2];
    int word_id_pos = max(1, (int) (tripstring.size() - 1));

    triphonize(tripstring, triphones);

    for (auto triit = triphones.begin(); triit != triphones.end(); ++triit) {
        DecoderGraph::TriphoneNode trin;
        trin.hmm_id = dg.m_hmm_map[*triit];
        nodes.push_back(trin);
    }

    DecoderGraph::TriphoneNode trin;
    trin.subword_id = dg.m_subword_map[subword];
    nodes.insert(nodes.begin() + word_id_pos, trin);
}


void get_hmm_states(DecoderGraph &dg, const vector<string> &triphones,
        vector<int> &states) {
    for (auto tit = triphones.begin(); tit != triphones.end(); ++tit) {
        int hmm_index = dg.m_hmm_map[*tit];
        Hmm &hmm = dg.m_hmms[hmm_index];
        for (unsigned int sidx = 2; sidx < hmm.states.size(); ++sidx)
            states.push_back(hmm.states[sidx].model);
    }
}

void get_hmm_states(DecoderGraph &dg, string word, vector<int> &states) {
    vector<string> triphones;
    triphonize(dg, word, triphones);
    get_hmm_states(dg, triphones, states);
}

void get_hmm_states_cw(DecoderGraph &dg, string wrd1, string wrd2,
        vector<int> &states) {
    string phonestring;
    vector<string> triphones;

    for (auto swit = dg.m_word_segs[wrd1].begin();
            swit != dg.m_word_segs[wrd1].end(); ++swit)
        for (auto trit = dg.m_lexicon[*swit].begin();
                trit != dg.m_lexicon[*swit].end(); ++trit)
            phonestring += string(1, (*trit)[2]);

    for (auto swit = dg.m_word_segs[wrd2].begin();
            swit != dg.m_word_segs[wrd2].end(); ++swit)
        for (auto trit = dg.m_lexicon[*swit].begin();
                trit != dg.m_lexicon[*swit].end(); ++trit)
            phonestring += string(1, (*trit)[2]);

    triphonize(phonestring, triphones);
    get_hmm_states(dg, triphones, states);
}

void find_successor_word(vector<DecoderGraph::Node> &nodes,
        set<pair<int, int> > &matches, int word_id, node_idx_t node_idx,
        int depth) {
    DecoderGraph::Node &node = nodes[node_idx];
    if (depth > 0) {
        if (node.word_id == word_id) {
            matches.insert(make_pair(node_idx, depth));
            return;
        } else if (node.word_id != -1)
            return;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (*ait == node_idx)
            continue;
        find_successor_word(nodes, matches, word_id, *ait, depth + 1);
    }
}

bool assert_path(DecoderGraph &dg, vector<DecoderGraph::Node> &nodes,
        deque<int> states, deque<string> subwords, node_idx_t node_idx) {
    DecoderGraph::Node &curr_node = nodes[node_idx];

    if (curr_node.hmm_state != -1) {
        if (states.size() == 0)
            return false;
        if (states.back() != curr_node.hmm_state)
            return false;
        else
            states.pop_back();
        if (states.size() == 0 && subwords.size() == 0)
            return true;
    }

    if (curr_node.word_id != -1) {
        if (subwords.size() == 0)
            return false;
        if (subwords.back() != dg.m_subwords[curr_node.word_id])
            return false;
        else
            subwords.pop_back();
        if (states.size() == 0 && subwords.size() == 0)
            return true;
    }

    for (auto ait = curr_node.arcs.begin(); ait != curr_node.arcs.end();
            ++ait) {
        if (*ait == node_idx)
            continue;
        bool retval = assert_path(dg, nodes, states, subwords, *ait);
        if (retval)
            return true;
    }

    return false;
}

bool assert_path(DecoderGraph &dg, vector<DecoderGraph::Node> &nodes,
        vector<string> &triphones, vector<string> &subwords, bool debug) {
    deque<int> dstates;
    deque<string> dwords;

    for (auto tit = triphones.begin(); tit != triphones.end(); ++tit) {
        int hmm_index = dg.m_hmm_map[*tit];
        Hmm &hmm = dg.m_hmms[hmm_index];
        for (auto sit = hmm.states.begin(); sit != hmm.states.end(); ++sit)
            if (sit->model >= 0)
                dstates.push_front(sit->model);
    }
    for (auto wit = subwords.begin(); wit != subwords.end(); ++wit)
        dwords.push_front(*wit);

    if (debug) {
        cerr << "expecting hmm states: " << endl;
        for (auto it = dstates.rbegin(); it != dstates.rend(); ++it)
            cerr << " " << *it;
        cerr << endl;
        cerr << "expecting subwords: " << endl;
        for (auto it = subwords.begin(); it != subwords.end(); ++it)
            cerr << " " << *it;
        cerr << endl;

        cerr << "expecting states: " << endl;
        for (auto dit = dstates.rbegin(); dit != dstates.rend(); ++dit)
            cerr << " " << *dit;
        cerr << endl;
    }

    return assert_path(dg, nodes, dstates, dwords, START_NODE);
}

bool assert_word_pair_crossword(DecoderGraph &dg,
        vector<DecoderGraph::Node> &nodes, string word1, string word2,
        bool debug) {
    if (dg.m_lexicon.find(word1) == dg.m_word_segs.end())
        return false;
    if (dg.m_lexicon.find(word2) == dg.m_word_segs.end())
        return false;

    string phonestring;
    vector<string> triphones;
    vector<string> subwords;

    for (auto swit = dg.m_word_segs[word1].begin();
            swit != dg.m_word_segs[word1].end(); ++swit) {
        subwords.push_back(*swit);
        for (auto trit = dg.m_lexicon[*swit].begin();
                trit != dg.m_lexicon[*swit].end(); ++trit)
            phonestring += string(1, (*trit)[2]);
    }

    for (auto swit = dg.m_word_segs[word2].begin();
            swit != dg.m_word_segs[word2].end(); ++swit) {
        subwords.push_back(*swit);
        for (auto trit = dg.m_lexicon[*swit].begin();
                trit != dg.m_lexicon[*swit].end(); ++trit)
            phonestring += string(1, (*trit)[2]);
    }

    triphonize(phonestring, triphones);

    if (debug) {
        cerr << endl;
        for (auto trit = triphones.begin(); trit != triphones.end(); ++trit)
            cerr << " " << *trit;
        cerr << endl;
        for (auto swit = subwords.begin(); swit != subwords.end(); ++swit)
            cerr << " " << *swit;
        cerr << endl;
    }

    return assert_path(dg, nodes, triphones, subwords, debug);
}

bool assert_words(DecoderGraph &dg, vector<DecoderGraph::Node> &nodes,
        bool debug) {
    for (auto sit = dg.m_word_segs.begin(); sit != dg.m_word_segs.end();
            ++sit) {
        vector<string> triphones;
        triphonize(dg, sit->first, triphones);
        bool result = assert_path(dg, nodes, triphones, sit->second, false);
        if (!result) {
            if (debug)
                cerr << endl << "no path for word: " << sit->first << endl;
            return false;
        }
    }
    return true;
}

bool assert_words(DecoderGraph &dg, vector<DecoderGraph::Node> &nodes,
        map<string, vector<string> > &triphonized_words, bool debug) {
    for (auto sit = dg.m_word_segs.begin(); sit != dg.m_word_segs.end();
            ++sit) {
        bool result = assert_path(dg, nodes, triphonized_words[sit->first],
                sit->second, false);
        if (!result) {
            cerr << "error, word: " << sit->first << " not found" << endl;
            return false;
        }
    }
    return true;
}

bool assert_word_pairs(DecoderGraph &dg, std::vector<DecoderGraph::Node> &nodes,
        bool debug) {
    for (auto fit = dg.m_word_segs.begin(); fit != dg.m_word_segs.end();
            ++fit) {
        for (auto sit = dg.m_word_segs.begin(); sit != dg.m_word_segs.end();
                ++sit) {
            if (debug)
                cerr << endl << "checking word pair: " << fit->first << " - "
                        << sit->first << endl;
            bool result = assert_word_pair_crossword(dg, nodes, fit->first,
                    sit->first, debug);
            if (!result) {
                cerr << endl << "word pair: " << fit->first << " - "
                        << sit->first << " not found" << endl;
                return false;
            }
        }
    }
    return true;
}

bool assert_word_pairs(DecoderGraph &dg, std::vector<DecoderGraph::Node> &nodes,
        int num_pairs, bool debug) {
    int wp_count = 0;
    while (wp_count < num_pairs) {
        int rand1 = rand() % dg.m_word_segs.size();
        int rand2 = rand() % dg.m_word_segs.size();
        auto wit1 = dg.m_word_segs.begin();
        advance(wit1, rand1);
        auto wit2 = dg.m_word_segs.begin();
        advance(wit2, rand2);

        string first_word = wit1->first;
        string second_word = wit2->first;

        bool result = assert_word_pair_crossword(dg, nodes, first_word,
                second_word, debug);
        if (!result) {
            cerr << endl << "word pair: " << first_word << " - " << second_word
                    << " not found" << endl;
            return false;
        }

        wp_count++;
    }

    return true;
}

bool assert_transitions(DecoderGraph &dg, vector<DecoderGraph::Node> &nodes,
        bool debug) {
    for (unsigned int node_idx = 0; node_idx < nodes.size(); ++node_idx) {
        if (node_idx == END_NODE)
            continue;
        DecoderGraph::Node &node = nodes[node_idx];
        if (!node.arcs.size()) {
            if (debug)
                cerr << "Node " << node_idx << " has no transitions" << endl;
            return false;
        }

        if (node.hmm_state == -1) {
            for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
                if (*ait == node_idx) {
                    if (debug)
                        cerr << "Node " << node_idx
                                << " self-transition in non-hmm-node " << endl;
                    return false;
                }
            }
            continue;
        }

        //HmmState &state = dg.m_hmm_states[node.hmm_state];
        bool self_transition = false;
        bool out_transition = false;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            if (*ait == node_idx)
                self_transition = true;
            else
                out_transition = true;
        }
        if (!self_transition) {
            if (debug)
                cerr << "Node " << node_idx << " has no self-transition"
                        << endl;
            return false;
        }
        if (!out_transition) {
            if (debug)
                cerr << "Node " << node_idx << " has no out-transition" << endl;
            return false;
        }
    }
    return true;
}

bool assert_subword_ids_left(DecoderGraph &dg,
        vector<DecoderGraph::Node> &nodes, bool debug) {
    dg.set_reverse_arcs_also_from_unreachable(nodes);

    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit) {
        if (nit->word_id == -1)
            continue;
        if (nit->reverse_arcs.size() == 1) {
            if (*(nit->reverse_arcs.begin()) == START_NODE)
                continue;
            if (nodes[*(nit->reverse_arcs.begin())].word_id != -1)
                continue;
            if (nodes[*(nit->reverse_arcs.begin())].arcs.size() > 1)
                continue;
            return false;
        }
    }

    return true;
}

bool assert_subword_ids_right(DecoderGraph &dg,
        vector<DecoderGraph::Node> &nodes, bool debug) {
    dg.set_reverse_arcs_also_from_unreachable(nodes);

    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit) {
        if (nit->word_id == -1)
            continue;
        if (nit->arcs.size() == 1) {
            if (*(nit->arcs.begin()) == END_NODE)
                continue;
            if (nodes[*(nit->arcs.begin())].word_id != -1)
                continue;
            if (nodes[*(nit->arcs.begin())].reverse_arcs.size() > 1)
                continue;
            return false;
        }
    }

    return true;
}

bool assert_no_double_arcs(vector<DecoderGraph::Node> &nodes) {
    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit) {
        set<int> targets;
        for (auto ait = nit->arcs.begin(); ait != nit->arcs.end(); ++ait) {
            if (targets.find(*ait) != targets.end())
                return false;
            targets.insert(*ait);
        }
    }

    return true;
}

bool assert_no_duplicate_word_ids(DecoderGraph &dg,
        vector<DecoderGraph::Node> &nodes) {
    bool retval = true;
    for (unsigned int i = 0; i < dg.m_subwords.size(); i++) {
        set<pair<int, int> > results;
        find_successor_word(nodes, results, i, START_NODE);
        if (results.size() > 1 && dg.m_subwords[i].length() > 1) {
            cerr << results.size() << " matches for subword: "
                    << dg.m_subwords[i] << endl;
            retval = false;
        }
    }
    return retval;
}

bool assert_only_segmented_words(DecoderGraph &dg,
        vector<DecoderGraph::Node> &nodes, bool debug, deque<int> states,
        deque<int> subwords, int node_idx) {
    if (node_idx == END_NODE) {

        if (debug) {
            cerr << "found subwords: " << endl;
            for (auto swit = subwords.begin(); swit != subwords.end(); ++swit)
                cerr << dg.m_subwords[*swit] << " ";
            cerr << "found states: " << endl;
            for (auto stit = states.begin(); stit != states.end(); ++stit)
                cerr << *stit << " ";
        }

        string wrd;
        for (auto swit = subwords.begin(); swit != subwords.end(); ++swit)
            wrd += dg.m_subwords[*swit];
        if (dg.m_word_segs.find(wrd) == dg.m_word_segs.end())
            return false;

        auto swit = subwords.begin();
        auto eswit = dg.m_word_segs[wrd].begin();
        while (swit != subwords.end()) {
            if (dg.m_subwords[*swit] != *eswit)
                return false;
            swit++;
            eswit++;
        }

        vector<int> expected_states;
        get_hmm_states(dg, wrd, expected_states);
        if (states.size() != expected_states.size())
            return false;
        auto sit = states.begin();
        auto esit = expected_states.begin();
        while (sit != states.end()) {
            if ((*sit) != (*esit))
                return false;
            sit++;
            esit++;
        }

        return true;
    }

    DecoderGraph::Node &node = nodes[node_idx];
    if (node.hmm_state != -1)
        states.push_back(node.hmm_state);
    if (node.word_id != -1)
        subwords.push_back(node.word_id);
    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (nodes[*ait].flags)
            continue;
        bool rv = assert_only_segmented_words(dg, nodes, debug, states,
                subwords, *ait);
        if (!rv)
            return false;
    }

    return true;
}

bool assert_only_segmented_cw_word_pairs(DecoderGraph &dg,
        vector<DecoderGraph::Node> &nodes, deque<int> states,
        deque<int> subwords, int node_idx, bool cw_visited) {
    if (node_idx == END_NODE) {

        string wrd1, wrd2;
        auto swit = subwords.begin();
        while (true) {
            if (*swit == -1)
                break;
            wrd1 += dg.m_subwords[*swit];
            swit++;
        }
        if (dg.m_word_segs.find(wrd1) == dg.m_word_segs.end())
            return false;
        swit++;
        while (swit != subwords.end()) {
            wrd2 += dg.m_subwords[*swit];
            swit++;
        }
        if (dg.m_word_segs.find(wrd2) == dg.m_word_segs.end())
            return false;

        swit = subwords.begin();
        unsigned int eswi = 0;
        while (*swit != -1) {
            if (dg.m_word_segs[wrd1][eswi] != dg.m_subwords[*swit])
                return false;
            swit++;
            eswi++;
        }
        if (eswi != dg.m_word_segs[wrd1].size())
            return false;
        swit++;
        eswi = 0;
        while (swit != subwords.end()) {
            if (dg.m_word_segs[wrd2][eswi] != dg.m_subwords[*swit])
                return false;
            swit++;
            eswi++;
        }
        if (eswi != dg.m_word_segs[wrd2].size())
            return false;

        vector<int> expected_states;
        get_hmm_states_cw(dg, wrd1, wrd2, expected_states);
        if (states.size() != expected_states.size())
            return false;
        auto sit = states.begin();
        auto esit = expected_states.begin();
        while (sit != states.end()) {
            if ((*sit) != (*esit))
                return false;
            sit++;
            esit++;
        }

        return true;
    }

    DecoderGraph::Node &node = nodes[node_idx];
    if (node.hmm_state != -1)
        states.push_back(node.hmm_state);
    if (node.word_id != -1)
        subwords.push_back(node.word_id);
    if (node.flags && !cw_visited) {
        subwords.push_back(-1);
        cw_visited = true;
    }

    bool cw_entry_found = false;
    bool non_cw_entry_found = false;
    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (nodes[*ait].flags)
            cw_entry_found = true;
        else
            non_cw_entry_found = true;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        DecoderGraph::Node &target_node = nodes[*ait];
        if (cw_visited && non_cw_entry_found && target_node.flags)
            continue;
        if (!cw_visited && cw_entry_found && !target_node.flags)
            continue;
        bool rv = assert_only_segmented_cw_word_pairs(dg, nodes, states,
                subwords, *ait, cw_visited);
        if (!rv)
            return false;
    }

    return true;
}

void tie_state_prefixes(vector<DecoderGraph::Node> &nodes,
        bool stop_propagation) {
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);
    tie_state_prefixes(nodes, processed_nodes, stop_propagation, START_NODE);
    prune_unreachable_nodes(nodes);
}

void tie_state_prefixes(vector<DecoderGraph::Node> &nodes,
        set<node_idx_t> &processed_nodes, bool stop_propagation,
        node_idx_t node_idx) {
    if (node_idx == END_NODE)
        return;
    if (processed_nodes.find(node_idx) != processed_nodes.end())
        return;
    processed_nodes.insert(node_idx);
    DecoderGraph::Node &nd = nodes[node_idx];
    set<unsigned int> temp_arcs = nd.arcs;

    map<pair<int, set<unsigned int> >, set<unsigned int> > targets;
    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait) {
        int target_hmm = nodes[*ait].hmm_state;
        if (target_hmm != -1)
            targets[make_pair(target_hmm, nodes[*ait].reverse_arcs)].insert(
                    *ait);
    }

    bool arcs_removed = false;
    for (auto tit = targets.begin(); tit != targets.end(); ++tit) {
        if (tit->second.size() == 1)
            continue;

        auto nit = tit->second.begin();
        int tied_node_idx = *nit;
        nit++;
        while (nit != tit->second.end()) {
            int curr_node_idx = *nit;
            tied_node_idx = merge_nodes(nodes, tied_node_idx, curr_node_idx);
            arcs_removed = true;
            nit++;
        }
    }

    if (stop_propagation && !arcs_removed)
        return;

    for (auto arcit = temp_arcs.begin(); arcit != temp_arcs.end(); ++arcit)
        tie_state_prefixes(nodes, processed_nodes, stop_propagation, *arcit);
}

void tie_word_id_prefixes(vector<DecoderGraph::Node> &nodes,
        bool stop_propagation) {
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);
    tie_word_id_prefixes(nodes, processed_nodes, stop_propagation, START_NODE);
    prune_unreachable_nodes(nodes);
}

void tie_word_id_prefixes(vector<DecoderGraph::Node> &nodes,
        set<node_idx_t> &processed_nodes, bool stop_propagation,
        node_idx_t node_idx) {
    if (node_idx == END_NODE)
        return;
    if (processed_nodes.find(node_idx) != processed_nodes.end())
        return;
    processed_nodes.insert(node_idx);
    DecoderGraph::Node &nd = nodes[node_idx];
    set<unsigned int> temp_arcs = nd.arcs;

    map<pair<int, set<unsigned int> >, set<unsigned int> > targets;
    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait) {
        int word_id = nodes[*ait].word_id;
        if (word_id != -1)
            targets[make_pair(word_id, nodes[*ait].reverse_arcs)].insert(*ait);
    }

    bool arcs_removed = false;
    for (auto tit = targets.begin(); tit != targets.end(); ++tit) {
        if (tit->second.size() == 1)
            continue;

        auto nit = tit->second.begin();
        int tied_node_idx = *nit;
        nit++;
        while (nit != tit->second.end()) {
            int curr_node_idx = *nit;
            tied_node_idx = merge_nodes(nodes, tied_node_idx, curr_node_idx);
            arcs_removed = true;
            nit++;
        }
    }

    if (stop_propagation && !arcs_removed)
        return;

    for (auto arcit = temp_arcs.begin(); arcit != temp_arcs.end(); ++arcit)
        tie_word_id_prefixes(nodes, processed_nodes, stop_propagation, *arcit);
}

void tie_state_suffixes(vector<DecoderGraph::Node> &nodes,
        bool stop_propagation) {
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);
    tie_state_suffixes(nodes, processed_nodes, stop_propagation, END_NODE);
    prune_unreachable_nodes(nodes);
}

void tie_state_suffixes(vector<DecoderGraph::Node> &nodes,
        set<node_idx_t> &processed_nodes, bool stop_propagation,
        node_idx_t node_idx) {
    if (node_idx == START_NODE)
        return;
    if (processed_nodes.find(node_idx) != processed_nodes.end())
        return;
    if (debug)
        cerr << endl << "tying state: " << node_idx << endl;
    processed_nodes.insert(node_idx);
    DecoderGraph::Node &nd = nodes[node_idx];
    set<unsigned int> temp_arcs = nd.reverse_arcs;

    map<pair<int, set<unsigned int> >, set<unsigned int> > targets;
    for (auto ait = nd.reverse_arcs.begin(); ait != nd.reverse_arcs.end();
            ++ait) {
        int target_hmm = nodes[*ait].hmm_state;
        if (target_hmm != -1)
            targets[make_pair(target_hmm, nodes[*ait].arcs)].insert(*ait);
    }

    if (debug) {
        int target_count = 0;
        for (auto tit = targets.begin(); tit != targets.end(); ++tit)
            if (tit->second.size() > 1)
                target_count++;
        cerr << "total targets: " << target_count << endl;

        for (auto tit = targets.begin(); tit != targets.end(); ++tit) {
            if (tit->second.size() == 1)
                continue;
            cerr << "target: " << tit->first.first << " " << tit->second.size()
                    << endl;
            for (auto nit = tit->second.begin(); nit != tit->second.end();
                    ++nit)
                cerr << *nit << " ";
            cerr << endl;
        }
    }

    bool arcs_removed = false;
    for (auto tit = targets.begin(); tit != targets.end(); ++tit) {
        if (tit->second.size() == 1)
            continue;

        auto nit = tit->second.begin();
        int tied_node_idx = *nit;
        nit++;
        while (nit != tit->second.end()) {
            int curr_node_idx = *nit;
            tied_node_idx = merge_nodes(nodes, tied_node_idx, curr_node_idx);
            arcs_removed = true;
            nit++;
        }

        if (debug)
            cerr << "\tnew tied node idx: " << tied_node_idx << endl;
    }

    if (stop_propagation && !arcs_removed)
        return;

    for (auto arcit = temp_arcs.begin(); arcit != temp_arcs.end(); ++arcit)
        tie_state_suffixes(nodes, processed_nodes, stop_propagation, *arcit);
}

void tie_word_id_suffixes(vector<DecoderGraph::Node> &nodes,
        bool stop_propagation) {
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);
    tie_word_id_suffixes(nodes, processed_nodes, stop_propagation, END_NODE);
    prune_unreachable_nodes(nodes);
}

void tie_word_id_suffixes(vector<Node> &nodes, set<node_idx_t> &processed_nodes,
        bool stop_propagation, node_idx_t node_idx) {
    if (node_idx == START_NODE)
        return;
    if (processed_nodes.find(node_idx) != processed_nodes.end())
        return;
    processed_nodes.insert(node_idx);
    DecoderGraph::Node &nd = nodes[node_idx];
    set<unsigned int> temp_arcs = nd.reverse_arcs;

    map<pair<int, set<unsigned int> >, set<unsigned int> > targets;
    for (auto ait = nd.reverse_arcs.begin(); ait != nd.reverse_arcs.end();
            ++ait) {
        int word_id = nodes[*ait].word_id;
        if (word_id != -1)
            targets[make_pair(word_id, nodes[*ait].arcs)].insert(*ait);
    }

    bool arcs_removed = false;
    for (auto tit = targets.begin(); tit != targets.end(); ++tit) {
        if (tit->second.size() == 1)
            continue;

        auto nit = tit->second.begin();
        int tied_node_idx = *nit;
        nit++;
        while (nit != tit->second.end()) {
            int curr_node_idx = *nit;
            tied_node_idx = merge_nodes(nodes, tied_node_idx, curr_node_idx);
            arcs_removed = true;
            nit++;
        }
    }

    if (stop_propagation && !arcs_removed)
        return;

    for (auto arcit = temp_arcs.begin(); arcit != temp_arcs.end(); ++arcit)
        tie_word_id_suffixes(nodes, processed_nodes, stop_propagation, *arcit);
}

void print_graph(vector<DecoderGraph::Node> &nodes, vector<int> path,
        int node_idx) {
    path.push_back(node_idx);

    if (node_idx == END_NODE) {
        for (unsigned int i = 0; i < path.size(); i++) {
            if (nodes[path[i]].hmm_state != -1)
                //cout << " " << nodes[path[i]].hmm_state;
                cout << " " << path[i] << "(" << nodes[path[i]].hmm_state
                        << ")";
            if (nodes[path[i]].word_id != -1)
                //cout << " " << "(" << m_units[nodes[path[i]].word_id] << ")";
                cout << " " << path[i] << "("
                        << m_subwords[nodes[path[i]].word_id] << ")";
        }
        cout << endl << endl;
        return;
    }

    for (auto ait = nodes[node_idx].arcs.begin();
            ait != nodes[node_idx].arcs.end(); ++ait)
        print_graph(nodes, path, *ait);
}

void print_graph(vector<DecoderGraph::Node> &nodes) {
    vector<int> path;
    print_graph(nodes, path, START_NODE);
}

void print_dot_digraph(vector<DecoderGraph::Node> &nodes, ostream &fstr) {
    set<node_idx_t> node_idxs;
    //reachable_graph_nodes(nodes, node_idxs);
    for (unsigned int i = 0; i < nodes.size(); i++)
        node_idxs.insert(i);

    fstr << "digraph {" << endl << endl;
    fstr << "\tnode [shape=ellipse,fontsize=30,fixedsize=false,width=0.95];"
            << endl;
    fstr << "\tedge [fontsize=12];" << endl;
    fstr << "\trankdir=LR;" << endl << endl;

    for (auto it = node_idxs.begin(); it != node_idxs.end(); ++it) {
        DecoderGraph::Node &nd = nodes[*it];
        fstr << "\t" << *it;
        if (*it == START_NODE)
            fstr << " [label=\"start\"]" << endl;
        else if (*it == END_NODE)
            fstr << " [label=\"end\"]" << endl;
        else if (nd.hmm_state != -1 && nd.word_id == -1)
            fstr << " [label=\"" << nd.hmm_state << "\"]" << endl;
        else if (nd.hmm_state == -1 && nd.word_id != -1)
            fstr << " [label=\"" << m_subwords[nd.word_id] << "\"]" << endl;
        else if (nd.hmm_state != -1 && nd.word_id != -1)
            throw string("Problem");
        else
            fstr << " [label=\"" << *it << ":dummy\"]" << endl;
    }

    fstr << endl;
    for (auto nit = node_idxs.begin(); nit != node_idxs.end(); ++nit) {
        DecoderGraph::Node &node = nodes[*nit];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            fstr << "\t" << *nit << " -> " << *ait << endl;
    }
    fstr << "}" << endl;
}

void reachable_graph_nodes(vector<DecoderGraph::Node> &nodes,
        set<node_idx_t> &node_idxs, node_idx_t node_idx) {
    node_idxs.insert(node_idx);
    for (auto ait = nodes[node_idx].arcs.begin();
            ait != nodes[node_idx].arcs.end(); ++ait)
        if (node_idx != *ait && node_idxs.find(*ait) == node_idxs.end())
            reachable_graph_nodes(nodes, node_idxs, *ait);
}

void set_reverse_arcs_also_from_unreachable(vector<DecoderGraph::Node> &nodes) {
    clear_reverse_arcs(nodes);
    for (unsigned int i = 0; i < nodes.size(); ++i) {
        Node &node = nodes[i];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            nodes[*ait].reverse_arcs.insert(i);
    }
}

void set_reverse_arcs(vector<DecoderGraph::Node> &nodes) {

    clear_reverse_arcs(nodes);
    set<int> processed_nodes;
    set_reverse_arcs(nodes, START_NODE, processed_nodes);
}

void set_reverse_arcs(vector<DecoderGraph::Node> &nodes, int node_idx,
        set<int> &processed_nodes) {
    if (processed_nodes.find(node_idx) != processed_nodes.end())
        return;
    processed_nodes.insert(node_idx);

    for (auto ait = nodes[node_idx].arcs.begin();
            ait != nodes[node_idx].arcs.end(); ++ait) {
        nodes[*ait].reverse_arcs.insert(node_idx);
        set_reverse_arcs(nodes, *ait, processed_nodes);
    }
}

void clear_reverse_arcs(vector<DecoderGraph::Node> &nodes) {
    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit)
        nit->reverse_arcs.clear();
}

int merge_nodes(vector<DecoderGraph::Node> &nodes, int node_idx_1,
        int node_idx_2) {
    if (node_idx_1 == node_idx_2)
        throw string("Merging same nodes.");

    DecoderGraph::Node &merged_node = nodes[node_idx_1];
    DecoderGraph::Node &removed_node = nodes[node_idx_2];
    removed_node.hmm_state = -1;
    removed_node.word_id = -1;

    for (auto ait = removed_node.arcs.begin(); ait != removed_node.arcs.end();
            ++ait) {
        merged_node.arcs.insert(*ait);
        nodes[*ait].reverse_arcs.erase(node_idx_2);
        nodes[*ait].reverse_arcs.insert(node_idx_1);
    }

    for (auto ait = removed_node.reverse_arcs.begin();
            ait != removed_node.reverse_arcs.end(); ++ait) {
        merged_node.reverse_arcs.insert(*ait);
        nodes[*ait].arcs.erase(node_idx_2);
        nodes[*ait].arcs.insert(node_idx_1);
    }
    removed_node.arcs.clear();
    removed_node.reverse_arcs.clear();

    return node_idx_1;
}

void connect_end_to_start_node(vector<DecoderGraph::Node> &nodes) {
    nodes[END_NODE].arcs.insert(START_NODE);
}

void write_graph(vector<DecoderGraph::Node> &nodes, string fname) {
    std::ofstream outf(fname);
    outf << nodes.size() << endl;
    ;
    for (unsigned int i = 0; i < nodes.size(); i++)
        outf << "n " << i << " " << nodes[i].hmm_state << " "
                << nodes[i].word_id << " " << nodes[i].arcs.size() << " "
                << nodes[i].flags << endl;
    for (unsigned int i = 0; i < nodes.size(); i++) {
        DecoderGraph::Node &node = nodes[i];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            outf << "a " << i << " " << *ait << endl;
    }
}

int connect_triphone(DecoderGraph &dg,
        vector<DecoderGraph::Node> &nodes,
        string triphone,
        node_idx_t node_idx)
{
    int hmm_index = dg.m_hmm_map[triphone];
    return connect_triphone(dg, nodes, hmm_index, node_idx);
}

int connect_triphone(DecoderGraph &dg,
        vector<DecoderGraph::Node> &nodes,
        int hmm_index,
        node_idx_t node_idx)
{
    Hmm &hmm = dg.m_hmms[hmm_index];

    for (unsigned int sidx = 2; sidx < hmm.states.size(); ++sidx) {
        nodes.resize(nodes.size() + 1);
        nodes.back().hmm_state = hmm.states[sidx].model;
        nodes[node_idx].arcs.insert(nodes.size() - 1);
        node_idx = nodes.size() - 1;
    }

    return node_idx;
}

int connect_word(DecoderGraph &dg,
        vector<DecoderGraph::Node> &nodes,
        string word,
        node_idx_t node_idx)
{
    nodes.resize(nodes.size() + 1);
    nodes.back().word_id = dg.m_subword_map[word];
    node_idx = nodes.size() - 1;
    return node_idx;
}

int reachable_graph_nodes(vector<DecoderGraph::Node> &nodes) {
    set<node_idx_t> node_idxs;
    reachable_graph_nodes(nodes, node_idxs, START_NODE);
    return node_idxs.size();
}

void prune_unreachable_nodes(vector<DecoderGraph::Node> &nodes) {
    vector<DecoderGraph::Node> pruned_nodes;
    map<node_idx_t, node_idx_t> index_mapping;
    set<node_idx_t> old_node_idxs;
    reachable_graph_nodes(nodes, old_node_idxs, START_NODE);
    int new_idx = 0;
    for (auto nit = old_node_idxs.begin(); nit != old_node_idxs.end(); ++nit) {
        index_mapping[*nit] = new_idx;
        new_idx++;
    }

    pruned_nodes.resize(old_node_idxs.size());
    for (auto nit = old_node_idxs.begin(); nit != old_node_idxs.end(); ++nit) {
        pruned_nodes[index_mapping[*nit]] = nodes[*nit];
        DecoderGraph::Node &old_node = nodes[*nit];
        DecoderGraph::Node &new_node = pruned_nodes[index_mapping[*nit]];
        new_node.arcs.clear();
        for (auto ait = old_node.arcs.begin(); ait != old_node.arcs.end();
                ++ait) {
            new_node.arcs.insert(index_mapping[*ait]);
        }
    }

    nodes.swap(pruned_nodes);

    return;
}


void
add_hmm_self_transitions(std::vector<DecoderGraph::Node> &nodes)
{
    for (unsigned int i=0; i<nodes.size(); i++) {
        Node &node = nodes[i];
        if (node.hmm_state == -1) continue;
        node.arcs.insert(i);
    }
}


void
push_word_ids_left(vector<DecoderGraph::Node> &nodes,
                                 int &move_count,
                                 set<node_idx_t> &processed_nodes,
                                 node_idx_t node_idx,
                                 node_idx_t prev_node_idx,
                                 int subword_id)
{
    if (node_idx == START_NODE) return;
    processed_nodes.insert(node_idx);

    Node &node = nodes[node_idx];

    if (debug && subword_id != -1) {
        cerr << "push left, node_idx: " << node_idx << " prev_node_idx: " << prev_node_idx;
        cerr << " subword to move: " << m_subwords[subword_id] << endl;
        cerr << endl;
    }

    if (subword_id != -1 && prev_node_idx != node_idx) {
        Node &prev_node = nodes[prev_node_idx];
        swap(node.word_id, prev_node.word_id);
        swap(node.hmm_state, prev_node.hmm_state);
        move_count++;
    }

    if (node.reverse_arcs.size() == 1) subword_id = node.word_id;
    else subword_id = -1;

    for (auto ait = node.reverse_arcs.begin(); ait != node.reverse_arcs.end(); ++ait) {
        if (*ait == node_idx) throw string("Call push before setting self-transitions.");
        int temp_subword_id = subword_id;
        if (nodes[*ait].arcs.size() > 1) temp_subword_id = -1;
        if (nodes[*ait].word_id != -1) temp_subword_id = -1;
        if (processed_nodes.find(*ait) == processed_nodes.end())
            push_word_ids_left(nodes, move_count, processed_nodes, *ait, node_idx, temp_subword_id);
    }
}


void
push_word_ids_left(vector<DecoderGraph::Node> &nodes)
{
    set_reverse_arcs_also_from_unreachable(nodes);
    int move_count = 0;
    while (true) {
        set<node_idx_t> processed_nodes;
        push_word_ids_left(nodes, move_count, processed_nodes);
        if (move_count == 0) break;
        move_count = 0;
    }
}


void
push_word_ids_right(vector<DecoderGraph::Node> &nodes,
                                  int &move_count,
                                  set<node_idx_t> &processed_nodes,
                                  node_idx_t node_idx,
                                  node_idx_t prev_node_idx,
                                  int subword_id)
{
    if (node_idx == END_NODE) return;
    processed_nodes.insert(node_idx);

    DecoderGraph::Node &node = nodes[node_idx];

    if (debug && subword_id != -1) {
        cerr << "push right, node_idx: " << node_idx << " prev_node_idx: " << prev_node_idx;
        cerr << " subword to move: " << m_subwords[subword_id] << endl;
        cerr << endl;
    }

    if (subword_id != -1 && prev_node_idx != node_idx) {
        DecoderGraph::Node &prev_node = nodes[prev_node_idx];
        swap(node.word_id, prev_node.word_id);
        swap(node.hmm_state, prev_node.hmm_state);
        move_count++;
    }

    if (node.arcs.size() == 1) subword_id = node.word_id;
    else subword_id = -1;

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (*ait == node_idx) throw string("Call push before setting self-transitions.");
        int temp_subword_id = subword_id;
        if (nodes[*ait].reverse_arcs.size() > 1) temp_subword_id = -1;
        if (nodes[*ait].word_id != -1) temp_subword_id = -1;
        if (processed_nodes.find(*ait) == processed_nodes.end())
            push_word_ids_right(nodes, move_count, processed_nodes, *ait, node_idx, temp_subword_id);
    }
}


void
push_word_ids_right(vector<DecoderGraph::Node> &nodes)
{
    set_reverse_arcs_also_from_unreachable(nodes);
    int move_count = 0;
    while (true) {
        set<node_idx_t> processed_nodes;
        push_word_ids_right(nodes, move_count, processed_nodes);
        if (move_count == 0) break;
        move_count = 0;
    }
}



int
num_hmm_states(vector<DecoderGraph::Node> &nodes)
{
    set<node_idx_t> node_idxs;
    reachable_graph_nodes(nodes, node_idxs, START_NODE);
    int hmm_state_count = 0;
    for (auto iit = node_idxs.begin(); iit != node_idxs.end(); ++iit)
        if (nodes[*iit].hmm_state != -1) hmm_state_count++;
    return hmm_state_count;
}


int
num_subword_states(vector<DecoderGraph::Node> &nodes)
{
    set<node_idx_t> node_idxs;
    reachable_graph_nodes(nodes, node_idxs, START_NODE);
    int subword_state_count = 0;
    for (auto iit = node_idxs.begin(); iit != node_idxs.end(); ++iit)
        if (nodes[*iit].word_id != -1) subword_state_count++;
    return subword_state_count;
}



}
;
