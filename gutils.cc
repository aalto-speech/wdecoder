#include <cstdlib>
#include <sstream>

#include "defs.hh"
#include "gutils.hh"

using namespace std;


namespace gutils {

void triphonize(string word,
                vector<string> &triphones)
{
    string tword = "_" + word + "_";
    triphones.clear();
    for (int i=1; i<tword.length()-1; i++) {
        stringstream tstring;
        tstring << tword[i-1] << "-" << tword[i] << "+" << tword[i+1];
        triphones.push_back(tstring.str());
    }
}


void triphonize(DecoderGraph &dg,
                string word,
                vector<string> &triphones)
{
    if (dg.m_word_segs.find(word) != dg.m_word_segs.end()) {
        string tripstring;
        for (auto swit = dg.m_word_segs[word].begin(); swit != dg.m_word_segs[word].end(); ++swit) {
            vector<string> &triphones = dg.m_lexicon[*swit];
            for (auto tit = triphones.begin(); tit != triphones.end(); ++tit)
                tripstring += (*tit)[2];
        }
        triphonize(tripstring, triphones);
    }
    else triphonize(word, triphones);
}


void triphonize_all_words(DecoderGraph &dg,
                          map<string, vector<string> > &triphonized_words) {
    for (auto wit = dg.m_word_segs.begin(); wit != dg.m_word_segs.end(); ++wit) {
        vector<string> triphones;
        triphonize(dg, wit->first, triphones);
        triphonized_words[wit->first] = triphones;
    }
}


void get_hmm_states(DecoderGraph &dg,
                    const vector<string> &triphones,
                    vector<int> &states)
{
    for (auto tit = triphones.begin(); tit !=triphones.end(); ++tit) {
        int hmm_index = dg.m_hmm_map[*tit];
        Hmm &hmm = dg.m_hmms[hmm_index];
        for (int sidx = 2; sidx < hmm.states.size(); ++sidx)
            states.push_back(hmm.states[sidx].model);
    }
}


void get_hmm_states(DecoderGraph &dg,
                    string word,
                    vector<int> &states)
{
    vector<string> triphones;
    triphonize(dg, word, triphones);
    get_hmm_states(dg, triphones, states);
}


void
get_hmm_states_cw(DecoderGraph &dg,
                  string wrd1,
                  string wrd2,
                  vector<int> &states)
{
    string phonestring;
    vector<string> triphones;

    for (auto swit = dg.m_word_segs[wrd1].begin(); swit != dg.m_word_segs[wrd1].end(); ++swit)
        for (auto trit = dg.m_lexicon[*swit].begin(); trit != dg.m_lexicon[*swit].end(); ++trit)
            phonestring += string(1,(*trit)[2]);

    for (auto swit = dg.m_word_segs[wrd2].begin(); swit != dg.m_word_segs[wrd2].end(); ++swit)
        for (auto trit = dg.m_lexicon[*swit].begin(); trit != dg.m_lexicon[*swit].end(); ++trit)
            phonestring += string(1,(*trit)[2]);

    triphonize(phonestring, triphones);
    get_hmm_states(dg, triphones, states);
}


bool
assert_path(DecoderGraph &dg,
            vector<DecoderGraph::Node> &nodes,
            deque<int> states,
            deque<string> subwords,
            int node_idx)
{
    DecoderGraph::Node &curr_node = nodes[node_idx];

    if (curr_node.hmm_state != -1) {
        if (states.size() == 0) return false;
        if (states.back() != curr_node.hmm_state) return false;
        else states.pop_back();
        if (states.size() == 0 && subwords.size() == 0) return true;
    }

    if (curr_node.word_id != -1) {
        if (subwords.size() == 0) return false;
        if (subwords.back() != dg.m_units[curr_node.word_id]) return false;
        else subwords.pop_back();
        if (states.size() == 0 && subwords.size() == 0) return true;
    }

    for (auto ait = curr_node.arcs.begin(); ait != curr_node.arcs.end(); ++ait) {
        if (*ait == node_idx) continue;
        bool retval = assert_path(dg, nodes, states, subwords, *ait);
        if (retval) return true;
    }

    return false;
}


bool
assert_path(DecoderGraph &dg,
            vector<DecoderGraph::Node> &nodes,
            vector<string> &triphones,
            vector<string> &subwords,
            bool debug)
{
    deque<int> dstates;
    deque<string> dwords;

    for (auto tit = triphones.begin(); tit != triphones.end(); ++tit) {
        int hmm_index = dg.m_hmm_map[*tit];
        Hmm &hmm = dg.m_hmms[hmm_index];
        for (auto sit = hmm.states.begin(); sit != hmm.states.end(); ++sit)
            if (sit->model >= 0) dstates.push_front(sit->model);
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
        for (auto dit = dstates.rbegin(); dit !=dstates.rend(); ++dit)
            cerr << " " << *dit;
        cerr << endl;
    }

    return assert_path(dg, nodes, dstates, dwords, START_NODE);
}


bool
assert_word_pair_crossword(DecoderGraph &dg,
                           vector<DecoderGraph::Node> &nodes,
                           string word1,
                           string word2,
                           bool debug)
{
    if (dg.m_lexicon.find(word1) == dg.m_word_segs.end()) return false;
    if (dg.m_lexicon.find(word2) == dg.m_word_segs.end()) return false;

    string phonestring;
    vector<string> triphones;
    vector<string> subwords;

    for (auto swit = dg.m_word_segs[word1].begin(); swit != dg.m_word_segs[word1].end(); ++swit) {
        subwords.push_back(*swit);
        for (auto trit = dg.m_lexicon[*swit].begin(); trit != dg.m_lexicon[*swit].end(); ++trit)
            phonestring += string(1,(*trit)[2]);
    }

    for (auto swit = dg.m_word_segs[word2].begin(); swit != dg.m_word_segs[word2].end(); ++swit) {
        subwords.push_back(*swit);
        for (auto trit = dg.m_lexicon[*swit].begin(); trit != dg.m_lexicon[*swit].end(); ++trit)
            phonestring += string(1,(*trit)[2]);
    }

    triphonize(phonestring, triphones);

    if (debug) {
        cerr << endl;
        for (auto trit = triphones.begin(); trit !=triphones.end(); ++trit)
            cerr << " " << *trit;
        cerr << endl;
        for (auto swit = subwords.begin(); swit !=subwords.end(); ++swit)
            cerr << " " << *swit;
        cerr << endl;
    }

    return assert_path(dg, nodes, triphones, subwords, debug);
}


bool
assert_words(DecoderGraph &dg,
             vector<DecoderGraph::Node> &nodes,
             bool debug)
{
    for (auto sit=dg.m_word_segs.begin(); sit!=dg.m_word_segs.end(); ++sit) {
        vector<string> triphones;
        triphonize(dg, sit->first, triphones);
        bool result = assert_path(dg, nodes, triphones, sit->second, false);
        if (!result) {
            if (debug) cerr << endl << "no path for word: " << sit->first << endl;
            return false;
        }
    }
    return true;
}


bool
assert_words(DecoderGraph &dg,
             vector<DecoderGraph::Node> &nodes,
             map<string, vector<string> > &triphonized_words,
             bool debug)
{
    for (auto sit=dg.m_word_segs.begin(); sit!=dg.m_word_segs.end(); ++sit) {
        bool result = assert_path(dg, nodes, triphonized_words[sit->first], sit->second, false);
        if (!result) {
            cerr << "error, word: " << sit->first << " not found" << endl;
            return false;
        }
    }
    return true;
}


bool
assert_word_pairs(DecoderGraph &dg,
                  std::vector<DecoderGraph::Node> &nodes,
                  bool debug)
{
    for (auto fit=dg.m_word_segs.begin(); fit!=dg.m_word_segs.end(); ++fit) {
        for (auto sit=dg.m_word_segs.begin(); sit!=dg.m_word_segs.end(); ++sit) {
            if (debug) cerr << endl << "checking word pair: " << fit->first << " - " << sit->first << endl;
            bool result = assert_word_pair_crossword(dg, nodes, fit->first, sit->first, debug);
            if (!result) {
                cerr << endl << "word pair: " << fit->first << " - " << sit->first << " not found" << endl;
                return false;
            }
        }
    }
    return true;
}


bool
assert_word_pairs(DecoderGraph &dg,
                  std::vector<DecoderGraph::Node> &nodes,
                  int num_pairs,
                  bool debug)
{
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

        bool result = assert_word_pair_crossword(dg, nodes, first_word, second_word, debug);
        if (!result) {
            cerr << endl << "word pair: " << first_word << " - " << second_word << " not found" << endl;
            return false;
        }

        wp_count++;
    }

    return true;
}


bool
assert_transitions(DecoderGraph &dg,
                   vector<DecoderGraph::Node> &nodes,
                   bool debug)
{
    for (int node_idx = 0; node_idx < nodes.size(); ++node_idx) {
        if (node_idx == END_NODE) continue;
        DecoderGraph::Node &node = nodes[node_idx];
        if (!node.arcs.size()) {
            if (debug) cerr << "Node " << node_idx << " has no transitions" << endl;
            return false;
        }

        if (node.hmm_state == -1) {
            for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
                if (*ait == node_idx) {
                    if (debug) cerr << "Node " << node_idx << " self-transition in non-hmm-node " << endl;
                    return false;
                }
            }
            continue;
        }

        //HmmState &state = dg.m_hmm_states[node.hmm_state];
        bool self_transition = false;
        bool out_transition = false;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            if (*ait == node_idx) self_transition = true;
            else out_transition = true;
        }
        if (!self_transition) {
            if (debug) cerr << "Node " << node_idx << " has no self-transition" << endl;
            return false;
        }
        if (!out_transition) {
            if (debug) cerr << "Node " << node_idx << " has no out-transition" << endl;
            return false;
        }
    }
    return true;
}


bool
assert_subword_ids_left(DecoderGraph &dg,
                        vector<DecoderGraph::Node> &nodes,
                        bool debug)
{
    dg.set_reverse_arcs_also_from_unreachable(nodes);

    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit) {
        if (nit->word_id == -1) continue;
        if (nit->reverse_arcs.size() == 1) {
            if (*(nit->reverse_arcs.begin()) == START_NODE) continue;
            if (nodes[*(nit->reverse_arcs.begin())].word_id != -1) continue;
            if (nodes[*(nit->reverse_arcs.begin())].arcs.size() > 1) continue;
            return false;
        }
    }

    return true;
}


bool
assert_subword_ids_right(DecoderGraph &dg,
                         vector<DecoderGraph::Node> &nodes,
                         bool debug)
{
    dg.set_reverse_arcs_also_from_unreachable(nodes);

    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit) {
        if (nit->word_id == -1) continue;
        if (nit->arcs.size() == 1) {
            if (*(nit->arcs.begin()) == END_NODE) continue;
            if (nodes[*(nit->arcs.begin())].word_id != -1) continue;
            if (nodes[*(nit->arcs.begin())].reverse_arcs.size() > 1) continue;
            return false;
        }
    }

    return true;
}


bool
assert_no_double_arcs(vector<DecoderGraph::Node> &nodes)
{
    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit) {
        set<int> targets;
        for (auto ait = nit->arcs.begin(); ait != nit->arcs.end(); ++ait) {
            if (targets.find(*ait) != targets.end()) return false;
            targets.insert(*ait);
        }
    }

    return true;
}


bool
assert_only_segmented_words(DecoderGraph &dg,
                            vector<DecoderGraph::Node> &nodes,
                            bool debug,
                            deque<int> states,
                            deque<int> subwords,
                            int node_idx)
{
    if (node_idx == END_NODE) {

        if (debug) {
            cerr << "found subwords: " << endl;
            for (auto swit = subwords.begin(); swit != subwords.end(); ++swit)
                cerr << dg.m_units[*swit] << " ";
            cerr << "found states: " << endl;
            for (auto stit = states.begin(); stit != states.end(); ++stit)
                cerr << *stit << " ";
        }

        string wrd;
        for (auto swit = subwords.begin(); swit != subwords.end(); ++swit)
            wrd += dg.m_units[*swit];
        if (dg.m_word_segs.find(wrd) == dg.m_word_segs.end()) return false;

        auto swit = subwords.begin();
        auto eswit = dg.m_word_segs[wrd].begin();
        while (swit != subwords.end()) {
            if (dg.m_units[*swit] != *eswit) return false;
            swit++;
            eswit++;
        }

        vector<int> expected_states;
        get_hmm_states(dg, wrd, expected_states);
        if (states.size() != expected_states.size()) return false;
        auto sit = states.begin();
        auto esit = expected_states.begin();
        while (sit != states.end()) {
            if ((*sit) != (*esit)) return false;
            sit++;
            esit++;
        }

        return true;
    }

    DecoderGraph::Node &node = nodes[node_idx];
    if (node.hmm_state != -1) states.push_back(node.hmm_state);
    if (node.word_id != -1) subwords.push_back(node.word_id);
    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (nodes[*ait].flags) continue;
        bool rv = assert_only_segmented_words(dg, nodes, debug, states, subwords, *ait);
        if (!rv) return false;
    }

    return true;
}


bool
assert_only_segmented_cw_word_pairs(DecoderGraph &dg,
                                    vector<DecoderGraph::Node> &nodes,
                                    deque<int> states,
                                    deque<int> subwords,
                                    int node_idx,
                                    bool cw_visited)
{
    if (node_idx == END_NODE) {

        string wrd1, wrd2;
        auto swit = subwords.begin();
        while (true) {
            if (*swit == -1) break;
            wrd1 += dg.m_units[*swit];
            swit++;
        }
        if (dg.m_word_segs.find(wrd1) == dg.m_word_segs.end()) return false;
        swit++;
        while (swit != subwords.end()) {
            wrd2 += dg.m_units[*swit];
            swit++;
        }
        if (dg.m_word_segs.find(wrd2) == dg.m_word_segs.end()) return false;

        swit = subwords.begin();
        int eswi=0;
        while (*swit != -1) {
            if (dg.m_word_segs[wrd1][eswi] != dg.m_units[*swit]) return false;
            swit++;
            eswi++;
        }
        if (eswi != dg.m_word_segs[wrd1].size()) return false;
        swit++;
        eswi=0;
        while (swit != subwords.end()) {
            if (dg.m_word_segs[wrd2][eswi] != dg.m_units[*swit]) return false;
            swit++;
            eswi++;
        }
        if (eswi != dg.m_word_segs[wrd2].size()) return false;

        vector<int> expected_states;
        get_hmm_states_cw(dg, wrd1, wrd2, expected_states);
        if (states.size() != expected_states.size()) return false;
        auto sit = states.begin();
        auto esit = expected_states.begin();
        while (sit != states.end()) {
            if ((*sit) != (*esit)) return false;
            sit++;
            esit++;
        }

        return true;
    }

    DecoderGraph::Node &node = nodes[node_idx];
    if (node.hmm_state != -1) states.push_back(node.hmm_state);
    if (node.word_id != -1) subwords.push_back(node.word_id);
    if (node.flags && !cw_visited) {
        subwords.push_back(-1);
        cw_visited = true;
    }

    bool cw_entry_found = false;
    bool non_cw_entry_found = false;
    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (nodes[*ait].flags) cw_entry_found = true;
        else non_cw_entry_found = true;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        DecoderGraph::Node &target_node = nodes[*ait];
        if (cw_visited && non_cw_entry_found && target_node.flags) continue;
        if (!cw_visited && cw_entry_found && !target_node.flags) continue;
        bool rv = assert_only_segmented_cw_word_pairs(dg, nodes, states, subwords,
                  *ait, cw_visited);
        if (!rv) return false;
    }

    return true;
}

};
