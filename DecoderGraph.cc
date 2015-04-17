#include <sstream>
#include <algorithm>

#include "io.hh"
#include "defs.hh"
#include "NowayHmmReader.hh"
#include "DecoderGraph.hh"

using namespace std;


void
DecoderGraph::read_phone_model(string phnfname)
{
    ifstream phnf(phnfname);
    if (!phnf) throw string("Problem opening phone model.");

    int modelcount;
    NowayHmmReader::read(phnf, m_hmms, m_hmm_map, m_hmm_states, modelcount);

    set<string> sil_labels = { "_", "__", "_f", "_s" };
    m_states_per_phone = -1;
    for (unsigned int i=0; i<m_hmms.size(); i++) {
        Hmm &hmm = m_hmms[i];
        if (sil_labels.find(hmm.label) != sil_labels.end()) continue;
        if (m_states_per_phone == -1) m_states_per_phone = hmm.states.size();
        if (m_states_per_phone != (int)hmm.states.size()) {
            cerr << "Varying amount of states for normal phones." << endl;
            exit(1);
        }
    }
    m_states_per_phone -= 2;
}


void
DecoderGraph::read_noway_lexicon(string lexfname)
{
    ifstream lexf(lexfname);
    if (!lexf) throw string("Problem opening noway lexicon file.");

    string line;
    int linei = 0;
    while (getline(lexf, line)) {
        linei++;
        string unit;
        vector<string> phones;

        string phone;
        stringstream ss(line);
        ss >> unit;
        while (ss >> phone) phones.push_back(phone);

        auto leftp = unit.find("(");
        if (leftp != string::npos) {
            auto rightp = unit.find(")");
            if (rightp == string::npos) throw string("Problem reading line " + linei);
            double prob = ::atof(unit.substr(leftp+1, rightp-leftp-1).c_str());
            unit = unit.substr(0, leftp);
        }

        bool problem_phone = false;
        for (auto pit = phones.begin(); pit != phones.end(); ++pit) {
            if (m_hmm_map.find(*pit) == m_hmm_map.end())
                problem_phone = true;
        }
        if (problem_phone) {
            cerr << "Unknown phone in line " << linei << ", " << line << endl;
            continue;
        }

        if (m_subword_map.find(unit) == m_subword_map.end()) {
            m_subwords.push_back(unit);
            m_subword_map[unit] = m_subwords.size()-1;
        }
        m_lexicon[unit] = phones;
    }
}


void
DecoderGraph::read_words(string wordfname,
                         set<string> &words) const
{
    ifstream wordf(wordfname);
    if (!wordf) throw string("Problem opening word list.");

    string line;
    int linei = 1;
    while (getline(wordf, line)) {
        string word;
        stringstream ss(line);
        ss >> word;
        if (m_subword_map.find(word) == m_subword_map.end()) {
            cerr << "Word " + word + " not found in lexicon" << endl;
            continue;
        }
        words.insert(word);
        linei++;
    }
}


void
DecoderGraph::read_word_segmentations(string segfname,
                                      map<string, vector<string> > &word_segs) const
{
    ifstream segf(segfname);
    if (!segf) throw string("Problem opening word segmentations.");

    string line;
    int linei = 1;
    int faulty_words = 0;
    while (getline(segf, line)) {
        string word;
        string subword;

        stringstream ss(line);
        ss >> word;
        string concatenated;
        vector<string> tmp;

        bool ok = true;
        while (ss >> subword) {
            if (m_subword_map.find(subword) == m_subword_map.end()) {
                ok = false;
                faulty_words++;
            }
            tmp.push_back(subword);
            concatenated += subword;
        }
        if (concatenated != word) throw "Erroneous segmentation: " + concatenated;

        if (ok && tmp.size() > 0) word_segs[word] = tmp;

        linei++;
    }

    if (faulty_words > 0)
        cerr << faulty_words << " words were without a valid segmentation." << endl;
}


void
DecoderGraph::triphonize_phone_string(string pstring,
                                      vector<string> &triphones)
{
    string tword = "_" + pstring + "_";
    std::replace( tword.begin(), tword.end(), ' ', '_' );
    std::replace( tword.begin(), tword.end(), '\t', '_' );
    std::replace( tword.begin(), tword.end(), '\r', '_' );
    std::replace( tword.begin(), tword.end(), '\n', '_' );

    string::size_type pos;
    while ( tword.find("__") != string::npos ) {
        pos = tword.find("__");
        tword.replace( pos, 2, "_" );
    }

    triphones.clear();
    for (unsigned int i = 1; i < tword.length() - 1; i++) {
        stringstream tstring;
        char lc = tword[i-1];
        char rc = tword[i+1];
        if (i > 1 && lc == '_') lc = tword[i-2];
        if (i < tword.length()-2 && rc ==  '_') rc = tword[i+2];
        if (i > 1 && i < tword.length()-1 && tword[i] == '_')
            tstring << "_";
        else
            tstring << lc << "-" << tword[i] << "+" << rc;
        triphones.push_back(tstring.str());
    }
}


void
DecoderGraph::triphonize(map<string, vector<string> > &word_segs,
                         string word,
                         vector<string> &triphones) const
{
    if (word_segs.find(word) != word_segs.end()) {
        string tripstring;
        for (auto swit = word_segs[word].begin();
                swit != word_segs[word].end(); ++swit) {
            const vector<string> &temp_triphones = m_lexicon.at(*swit);
            for (auto tit = temp_triphones.begin(); tit != temp_triphones.end(); ++tit)
                tripstring += (*tit)[2];
        }
        triphonize_phone_string(tripstring, triphones);
    } else
        triphonize_phone_string(word, triphones);
}

void
DecoderGraph::triphonize(string word,
                         vector<string> &triphones) const
{
    string tripstring;
    const vector<string> &word_triphones = m_lexicon.at(word);
    for (auto tit = word_triphones.begin(); tit != word_triphones.end(); ++tit)
        tripstring += (*tit)[2];
    triphonize_phone_string(tripstring, triphones);
}

bool
DecoderGraph::triphonize(const vector<string> &word_seg,
                         vector<TriphoneNode> &nodes) const
{
    nodes.clear();

    string tripstring;
    vector<pair<int, int> > word_id_positions;
    vector<string> triphones;

    for (auto swit = word_seg.begin(); swit != word_seg.end(); ++swit) {
        if (m_lexicon.find(*swit) == m_lexicon.end())
             return false;

        const vector<string> &triphones = m_lexicon.at(*swit);
        for (auto tit = triphones.begin(); tit != triphones.end(); ++tit)
            tripstring += (*tit)[2];
        int word_id_pos = max(1, (int) (tripstring.size() - 1));
        word_id_positions.push_back(make_pair(m_subword_map.at(*swit), word_id_pos));
    }

    triphonize_phone_string(tripstring, triphones);

    for (auto triit = triphones.begin(); triit != triphones.end(); ++triit)
    {
        TriphoneNode trin;
        trin.hmm_id = m_hmm_map.at(*triit);
        nodes.push_back(trin);
    }

    if (nodes.size() == 0) return false;

    for (auto wit = word_id_positions.rbegin(); wit != word_id_positions.rend(); ++wit)
    {
        TriphoneNode trin;
        trin.subword_id = wit->first;
        nodes.insert(nodes.begin() + wit->second, trin);
    }

    return true;
}

void
DecoderGraph::triphonize_subword(const string &subword,
                                 vector<TriphoneNode> &nodes) const
{
    nodes.clear();

    const vector<string> &triphones = m_lexicon.at(subword);
    if (triphones.size() == 0) return;
    int word_id_pos = max(1, (int) (triphones.size() - 1));
    for (auto triit = triphones.begin(); triit != triphones.end(); ++triit) {
        TriphoneNode trin;
        trin.hmm_id = m_hmm_map.at(*triit);
        nodes.push_back(trin);
    }

    TriphoneNode trin;
    trin.subword_id = m_subword_map.at(subword);
    nodes.insert(nodes.begin() + word_id_pos, trin);
}

void
DecoderGraph::triphones_to_state_chain(vector<TriphoneNode> &triphone_nodes,
                                       vector<DecoderGraph::Node> &nodes) const
{
    for (auto triit = triphone_nodes.begin(); triit != triphone_nodes.end(); ++triit)
    {
        if (triit->subword_id != -1) {
            nodes.resize(nodes.size()+1);
            nodes.back().word_id = triit->subword_id;
        }
        else {
            const Hmm &hmm = m_hmms.at(triit->hmm_id);
            for (unsigned int sidx = 2; sidx < hmm.states.size(); ++sidx) {
                nodes.resize(nodes.size() + 1);
                nodes.back().hmm_state = hmm.states[sidx].model;
            }
        }
    }
}

void
DecoderGraph::add_nodes_to_tree(vector<DecoderGraph::Node> &nodes,
                                vector<DecoderGraph::Node> &new_nodes)
{
    int curr_node_idx = START_NODE;
    for (auto nnit=new_nodes.begin(); nnit != new_nodes.end(); ++nnit)
    {
        int word_id = nnit->word_id;
        int hmm_state = nnit->hmm_state;

        DecoderGraph::Node &curr_node = nodes[curr_node_idx];
        if (curr_node.lookahead == nullptr) curr_node.lookahead = new map<pair<int, int>, int>;
        int next_node = nodes[curr_node_idx].find_next(word_id, hmm_state);
        if (next_node == -1) {
            nodes.resize(nodes.size()+1);
            nodes.back().word_id = word_id;
            nodes.back().hmm_state = hmm_state;
            (*(nodes[curr_node_idx].lookahead))[make_pair(word_id, hmm_state)] = nodes.size()-1;
            curr_node_idx = nodes.size()-1;
        }
        else curr_node_idx = next_node;

        nodes[curr_node_idx].to_fanout.insert(nnit->to_fanout.begin(), nnit->to_fanout.end());
        nodes[curr_node_idx].from_fanin.insert(nnit->from_fanin.begin(), nnit->from_fanin.end());
    }

    DecoderGraph::Node &curr_node = nodes[curr_node_idx];
    curr_node.arcs.insert(END_NODE);
}

void
DecoderGraph::lookahead_to_arcs(vector<DecoderGraph::Node> &nodes)
{
    for (auto nit=nodes.begin(); nit != nodes.end(); ++nit) {
        if (nit->lookahead == nullptr) continue;
        DecoderGraph::Node &node = *nit;
        for (auto lait=(*(node.lookahead)).begin(); lait != (*(node.lookahead)).end(); ++lait)
            node.arcs.insert(lait->second);
        delete nit->lookahead;
    }
}


void
DecoderGraph::get_hmm_states(const vector<string> &triphones,
                             vector<int> &states) const
{
    for (auto tit = triphones.begin(); tit != triphones.end(); ++tit) {
        int hmm_index = m_hmm_map.at(*tit);
        const Hmm &hmm = m_hmms.at(hmm_index);
        for (unsigned int sidx = 2; sidx < hmm.states.size(); ++sidx)
            states.push_back(hmm.states[sidx].model);
    }
}

void
DecoderGraph::get_hmm_states(map<string, vector<string> > &word_segs,
                             string word,
                             vector<int> &states) const
{
    vector<string> triphones;
    triphonize(word_segs, word, triphones);
    get_hmm_states(triphones, states);
}

void
DecoderGraph::get_hmm_states_cw(map<string, vector<string> > &word_segs,
                                string wrd1,
                                string wrd2,
                                vector<int> &states) const
{
    string phonestring;
    vector<string> triphones;

    for (auto swit = word_segs[wrd1].begin();
            swit != word_segs[wrd1].end(); ++swit)
        for (auto trit = m_lexicon.at(*swit).begin();
                trit != m_lexicon.at(*swit).end(); ++trit)
            phonestring += string(1, (*trit)[2]);

    phonestring += "_";

    for (auto swit = word_segs[wrd2].begin();
            swit != word_segs[wrd2].end(); ++swit)
        for (auto trit = m_lexicon.at(*swit).begin();
                trit != m_lexicon.at(*swit).end(); ++trit)
            phonestring += string(1, (*trit)[2]);

    triphonize_phone_string(phonestring, triphones);
    get_hmm_states(triphones, states);
}

void
DecoderGraph::find_successor_word(vector<DecoderGraph::Node> &nodes,
                                  set<pair<int, int> > &matches,
                                  int word_id,
                                  node_idx_t node_idx,
                                  int depth)
{
    DecoderGraph::Node &node = nodes[node_idx];
    if (depth > 0) {
        if (node.word_id == word_id) {
            matches.insert(make_pair(node_idx, depth));
            return;
        } else if (node.word_id != -1)
            return;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (*ait == node_idx) continue;
        find_successor_word(nodes, matches, word_id, *ait, depth + 1);
    }
}

void
DecoderGraph::find_nodes_in_depth(vector<DecoderGraph::Node> &nodes,
                                  set<node_idx_t> &found_nodes,
                                  int target_depth,
                                  int curr_depth,
                                  node_idx_t curr_node)
{
    if (curr_depth == target_depth) {
        found_nodes.insert(curr_node);
        return;
    }

    DecoderGraph::Node &node = nodes[curr_node];
    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (*ait == curr_node) continue;
        find_nodes_in_depth(nodes, found_nodes, target_depth, curr_depth+1, *ait);
    }
}

void
find_nodes_in_depth_reverse(vector<DecoderGraph::Node> &nodes,
                            set<node_idx_t> &found_nodes,
                            int target_depth,
                            int curr_depth,
                            node_idx_t curr_node)
{
    if (curr_depth == target_depth) {
        found_nodes.insert(curr_node);
        return;
    }

    DecoderGraph::Node &node = nodes[curr_node];
    for (auto ait = node.reverse_arcs.begin(); ait != node.reverse_arcs.end(); ++ait) {
        if (*ait == curr_node) continue;
        find_nodes_in_depth_reverse(nodes, found_nodes, target_depth, curr_depth+1, *ait);
    }
}


bool
DecoderGraph::assert_path(vector<DecoderGraph::Node> &nodes,
                          deque<int> states,
                          deque<string> subwords,
                          node_idx_t node_idx)
{
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
        if (subwords.back() != m_subwords[curr_node.word_id])
            return false;
        else
            subwords.pop_back();
        if (states.size() == 0 && subwords.size() == 0)
            return true;
    }

    for (auto ait = curr_node.arcs.begin(); ait != curr_node.arcs.end(); ++ait) {
        if (*ait == node_idx) continue;
        bool retval = assert_path(nodes, states, subwords, *ait);
        if (retval) return true;
    }

    return false;
}

bool
DecoderGraph::assert_path(vector<DecoderGraph::Node> &nodes,
                          vector<string> &triphones,
                          vector<string> &subwords)
{
    deque<int> dstates;
    deque<string> dwords;

    for (auto tit = triphones.begin(); tit != triphones.end(); ++tit) {
        int hmm_index = m_hmm_map[*tit];
        Hmm &hmm = m_hmms[hmm_index];
        for (auto sit = hmm.states.begin(); sit != hmm.states.end(); ++sit)
            if (sit->model >= 0)
                dstates.push_front(sit->model);
    }
    for (auto wit = subwords.begin(); wit != subwords.end(); ++wit)
        dwords.push_front(*wit);

    return assert_path(nodes, dstates, dwords, START_NODE);
}

bool
DecoderGraph::assert_words(vector<DecoderGraph::Node> &nodes,
                           map<string, vector<string> > &word_segs)
{
    for (auto sit = word_segs.begin(); sit != word_segs.end(); ++sit) {
        vector<string> triphones;
        triphonize(word_segs, sit->first, triphones);
        bool result = assert_path(nodes, triphones, sit->second);
        if (!result) {
            cerr << "error, word: " << sit->first << " not found" << endl;
            return false;
        }
    }
    return true;
}

bool
DecoderGraph::assert_words(vector<DecoderGraph::Node> &nodes,
                           set<string> &words)
{
    for (auto sit = words.begin(); sit != words.end(); ++sit) {
        vector<string> triphones;
        triphonize(*sit, triphones);
        vector<string> lm_ids; lm_ids.push_back(*sit);
        bool result = assert_path(nodes, triphones, lm_ids);
        if (!result) {
            cerr << "error, word: " << *sit << " not found" << endl;
            return false;
        }
    }
    return true;
}

bool
DecoderGraph::assert_word_pair_crossword(vector<DecoderGraph::Node> &nodes,
                                         map<string, vector<string> > &word_segs,
                                         string word1,
                                         string word2,
                                         bool short_silence,
                                         bool wb_symbol)
{
    if (word_segs.find(word1) == word_segs.end())
        return false;
    if (word_segs.find(word2) == word_segs.end())
        return false;

    string phonestring;
    vector<string> triphones;
    vector<string> subwords;

    for (auto swit = word_segs[word1].begin();
         swit != word_segs[word1].end(); ++swit) {
        subwords.push_back(*swit);
        for (auto trit = m_lexicon[*swit].begin();
            trit != m_lexicon[*swit].end(); ++trit)
                phonestring += string(1, (*trit)[2]);
    }

    if (short_silence) phonestring += "_";
    if (wb_symbol) subwords.push_back("<w>");

    for (auto swit = word_segs[word2].begin();
            swit != word_segs[word2].end(); ++swit) {
        subwords.push_back(*swit);
        for (auto trit = m_lexicon[*swit].begin();
                trit != m_lexicon[*swit].end(); ++trit)
            phonestring += string(1, (*trit)[2]);
    }

    triphonize_phone_string(phonestring, triphones);

    return assert_path(nodes, triphones, subwords);
}

bool
DecoderGraph::assert_word_pairs(vector<DecoderGraph::Node> &nodes,
                                map<string, vector<string> > &word_segs,
                                bool short_silence,
                                bool wb_symbol)
{
    for (auto fit = word_segs.begin(); fit != word_segs.end(); ++fit)
        for (auto sit = word_segs.begin(); sit != word_segs.end(); ++sit)
        {
            bool result = assert_word_pair_crossword(nodes, word_segs,
                          fit->first, sit->first, short_silence, wb_symbol);
            if (!result) {
                cerr << endl << "word pair: " << fit->first << " - "
                     << sit->first << " not found" << endl;
                return false;
            }
        }

    return true;
}

bool
DecoderGraph::assert_word_pairs(vector<DecoderGraph::Node> &nodes,
                                map<string, vector<string> > &word_segs,
                                int num_pairs,
                                bool short_silence,
                                bool wb_symbol)
{
    int wp_count = 0;
    while (wp_count < num_pairs) {
        int rand1 = rand() % word_segs.size();
        int rand2 = rand() % word_segs.size();
        auto wit1 = word_segs.begin();
        advance(wit1, rand1);
        auto wit2 = word_segs.begin();
        advance(wit2, rand2);

        string first_word = wit1->first;
        string second_word = wit2->first;

        bool result = assert_word_pair_crossword(nodes, word_segs,
                      first_word, second_word, short_silence, wb_symbol);
        if (!result) {
            cerr << endl << "word pair: " << first_word << " - " << second_word
                 << " not found" << endl;
            return false;
        }

        wp_count++;
    }

    return true;
}

bool
DecoderGraph::assert_word_pairs(vector<DecoderGraph::Node> &nodes,
                                set<string> &words,
                                bool short_silence,
                                bool wb_symbol)
{
    map<string, vector<string> > word_segs;
    for (auto wit=words.begin(); wit != words.end(); ++wit)
        word_segs[*wit].push_back(*wit);

    return assert_word_pairs(nodes, word_segs, short_silence, wb_symbol);
}

bool
DecoderGraph::assert_word_pairs(vector<DecoderGraph::Node> &nodes,
                                set<string> &words,
                                int num_pairs,
                                bool short_silence,
                                bool wb_symbol)
{
    map<string, vector<string> > word_segs;
    for (auto wit=words.begin(); wit != words.end(); ++wit)
        word_segs[*wit].push_back(*wit);

    return assert_word_pairs(nodes, word_segs, num_pairs, short_silence, wb_symbol);
}

bool
DecoderGraph::assert_transitions(vector<DecoderGraph::Node> &nodes)
{
    for (unsigned int node_idx = 0; node_idx < nodes.size(); ++node_idx) {
        if (node_idx == END_NODE)
            continue;
        DecoderGraph::Node &node = nodes[node_idx];
        if (!node.arcs.size()) {
            cerr << "Node " << node_idx << " has no transitions" << endl;
            return false;
        }

        if (node.hmm_state == -1) {
            for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
                if (*ait == node_idx) {
                    cerr << "Node " << node_idx << " self-transition in non-hmm-node " << endl;
                    return false;
                }
            }
            continue;
        }

        bool self_transition = false;
        bool out_transition = false;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            if (*ait == node_idx)
                self_transition = true;
            else
                out_transition = true;
        }
        if (!self_transition) {
            cerr << "Node " << node_idx << " has no self-transition" << endl;
            return false;
        }
        if (!out_transition) {
            cerr << "Node " << node_idx << " has no out-transition" << endl;
            return false;
        }
    }
    return true;
}

bool
DecoderGraph::assert_subword_ids_left(vector<DecoderGraph::Node> &nodes)
{
    set_reverse_arcs_also_from_unreachable(nodes);

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

bool
DecoderGraph::assert_subword_ids_right(vector<DecoderGraph::Node> &nodes)
{
    set_reverse_arcs_also_from_unreachable(nodes);

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

bool
DecoderGraph::assert_no_duplicate_word_ids(vector<DecoderGraph::Node> &nodes)
{
    bool retval = true;
    for (unsigned int i = 0; i < m_subwords.size(); i++) {
        set<pair<int, int> > results;
        find_successor_word(nodes, results, i, START_NODE);
        if (results.size() > 1 && m_subwords[i].length() > 1) {
            cerr << results.size() << " matches for subword: "
                 << m_subwords[i] << endl;
            retval = false;
        }
    }
    return retval;
}

bool
DecoderGraph::assert_only_segmented_words(vector<DecoderGraph::Node> &nodes,
                                          map<string, vector<string> > &word_segs,
                                          deque<int> states,
                                          deque<int> subwords,
                                          int node_idx)
{
    if (node_idx == END_NODE) {

        string wrd;
        for (auto swit = subwords.begin(); swit != subwords.end(); ++swit)
            wrd += m_subwords[*swit];
        if (word_segs.find(wrd) == word_segs.end())
            return false;

        auto swit = subwords.begin();
        auto eswit = word_segs[wrd].begin();
        while (swit != subwords.end()) {
            if (m_subwords[*swit] != *eswit)
                return false;
            swit++;
            eswit++;
        }

        vector<int> expected_states;
        get_hmm_states(word_segs, wrd, expected_states);
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
        if (nodes[*ait].flags) continue;
        bool rv = assert_only_segmented_words(nodes, word_segs,
                                              states, subwords, *ait);
        if (!rv) return false;
    }

    return true;
}


bool
DecoderGraph::assert_only_words(vector<DecoderGraph::Node> &nodes,
                                set<string> &words)
{
    map<string, vector<string> > seg_words;
    for (auto wit = words.begin(); wit != words.end(); ++wit)
        seg_words[*wit].push_back(*wit);
    return assert_only_segmented_words(nodes, seg_words);
}


bool
DecoderGraph::assert_only_segmented_cw_word_pairs(vector<DecoderGraph::Node> &nodes,
                                                  map<string, vector<string> > &word_segs,
                                                  vector<int> states,
                                                  pair<vector<int>, vector<int>> subwords,
                                                  int node_idx,
                                                  bool cw_visited)
{
    if (node_idx == END_NODE) {

        // One phone word or subword, ok
        if (!cw_visited) return true;

        string wrd1, wrd2;
        vector<string> wrd1_seg, wrd2_seg;
        for (auto swit = subwords.first.begin(); swit != subwords.first.end(); ++swit) {
            wrd1 += m_subwords[*swit];
            wrd1_seg.push_back(m_subwords[*swit]);
        }
        if (word_segs.find(wrd1) == word_segs.end()) return false;
        if (word_segs.at(wrd1) != wrd1_seg) return false;

        for (auto swit = subwords.second.begin(); swit != subwords.second.end(); ++swit) {
            wrd2 += m_subwords[*swit];
            wrd2_seg.push_back(m_subwords[*swit]);
        }
        if (word_segs.find(wrd2) == word_segs.end()) return false;
        if (word_segs.at(wrd2) != wrd2_seg) return false;

        vector<int> expected_states;
        get_hmm_states_cw(word_segs, wrd1, wrd2, expected_states);
        if (expected_states != states) return false;

        return true;
    }

    DecoderGraph::Node &node = nodes[node_idx];
    if ((node.flags & NODE_CW) && !cw_visited) cw_visited = true;
    if (node.hmm_state != -1)
        states.push_back(node.hmm_state);
    if (node.word_id != -1) {
        if (cw_visited)
            subwords.second.push_back(node.word_id);
        else
            subwords.first.push_back(node.word_id);
    }

    bool cw_entry_found = false;
    bool non_cw_entry_found = false;
    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if ((nodes[*ait].flags) & NODE_CW) cw_entry_found = true;
        else non_cw_entry_found = true;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        DecoderGraph::Node &target_node = nodes[*ait];
        if (cw_visited && non_cw_entry_found && (target_node.flags & NODE_CW))
            continue;
        if (!cw_visited && cw_entry_found && !(target_node.flags & NODE_CW))
            continue;
        bool rv = assert_only_segmented_cw_word_pairs(nodes, word_segs,
                  states, subwords, *ait, cw_visited);
        if (!rv) return false;
    }

    return true;
}

bool
DecoderGraph::assert_only_cw_word_pairs(std::vector<DecoderGraph::Node> &nodes,
                                        std::set<std::string> &words)
{
    map<string, vector<string> > seg_words;
    for (auto wit = words.begin(); wit != words.end(); ++wit)
        seg_words[*wit].push_back(*wit);
    return assert_only_segmented_cw_word_pairs(nodes, seg_words);
}


void
DecoderGraph::tie_state_prefixes(vector<DecoderGraph::Node> &nodes,
                                 bool stop_propagation,
                                 node_idx_t node_idx)
{
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);
    tie_state_prefixes(nodes, processed_nodes, stop_propagation, node_idx);
    prune_unreachable_nodes(nodes);
}

void
DecoderGraph::tie_state_prefixes_cw(vector<DecoderGraph::Node> &nodes,
                                    map<string, int> &fanout,
                                    map<string, int> &fanin,
                                    bool stop_propagation)
{
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);

    set<node_idx_t> start_nodes;
    for (unsigned int i=0; i<nodes.size(); i++)
        if (nodes[i].flags & NODE_FAN_OUT_DUMMY)
            start_nodes.insert(i);

    for (auto snit = start_nodes.begin(); snit != start_nodes.end(); ++snit)
        tie_state_prefixes(nodes, processed_nodes, stop_propagation, *snit);
    prune_unreachable_nodes_cw(nodes, start_nodes, fanout, fanin);
}

void
DecoderGraph::tie_state_prefixes(vector<DecoderGraph::Node> &nodes,
                                 set<node_idx_t> &processed_nodes,
                                 bool stop_propagation,
                                 node_idx_t node_idx)
{
    if (processed_nodes.find(node_idx) != processed_nodes.end())
        return;
    processed_nodes.insert(node_idx);
    DecoderGraph::Node &nd = nodes[node_idx];

    map<pair<int, set<unsigned int> >, vector<unsigned int> > targets;
    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait) {
        int target_hmm = nodes[*ait].hmm_state;
        if (target_hmm != -1)
            targets[make_pair(target_hmm, nodes[*ait].reverse_arcs)].push_back(*ait);
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

    set<unsigned int> temp_arcs = nd.arcs;
    for (auto arcit = temp_arcs.begin(); arcit != temp_arcs.end(); ++arcit)
        tie_state_prefixes(nodes, processed_nodes, stop_propagation, *arcit);
}

void
DecoderGraph::tie_word_id_prefixes(vector<DecoderGraph::Node> &nodes,
                                   bool stop_propagation,
                                   node_idx_t node_idx)
{
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);
    tie_word_id_prefixes(nodes, processed_nodes, stop_propagation, node_idx);
    prune_unreachable_nodes(nodes);
}

void
DecoderGraph::tie_word_id_prefixes_cw(vector<DecoderGraph::Node> &nodes,
                                      map<string, int> &fanout,
                                      map<string, int> &fanin,
                                      bool stop_propagation)
{
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);

    set<node_idx_t> start_nodes;
    for (unsigned int i=0; i<nodes.size(); i++)
        if (nodes[i].flags & NODE_FAN_OUT_DUMMY)
            start_nodes.insert(i);

    for (auto snit = start_nodes.begin(); snit != start_nodes.end(); ++snit)
        tie_word_id_prefixes(nodes, processed_nodes, stop_propagation, *snit);
    prune_unreachable_nodes_cw(nodes, start_nodes, fanout, fanin);
}


void
DecoderGraph::tie_word_id_prefixes(vector<DecoderGraph::Node> &nodes,
                                   set<node_idx_t> &processed_nodes,
                                   bool stop_propagation,
                                   node_idx_t node_idx)
{
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

void
DecoderGraph::tie_state_suffixes(vector<DecoderGraph::Node> &nodes,
                                 bool stop_propagation,
                                 node_idx_t node_idx)
{
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);
    tie_state_suffixes(nodes, processed_nodes, stop_propagation, node_idx);
    prune_unreachable_nodes(nodes);
}

void
DecoderGraph::tie_state_suffixes_cw(vector<DecoderGraph::Node> &nodes,
                                    map<string, int> &fanout,
                                    map<string, int> &fanin,
                                    bool stop_propagation)
{
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);

    set<node_idx_t> start_nodes;
    set<node_idx_t> end_nodes;
    for (unsigned int i=0; i<nodes.size(); i++)
        if (nodes[i].flags & NODE_FAN_OUT_DUMMY)
            start_nodes.insert(i);
        else if (nodes[i].flags & NODE_FAN_IN_DUMMY)
            end_nodes.insert(i);

    for (auto enit = end_nodes.begin(); enit != end_nodes.end(); ++enit)
        tie_state_suffixes(nodes, processed_nodes, stop_propagation, *enit);
    prune_unreachable_nodes_cw(nodes, start_nodes, fanout, fanin);
}

void
DecoderGraph::tie_state_suffixes(vector<DecoderGraph::Node> &nodes,
                                 set<node_idx_t> &processed_nodes,
                                 bool stop_propagation,
                                 node_idx_t node_idx)
{
    if (processed_nodes.find(node_idx) != processed_nodes.end())
        return;
    processed_nodes.insert(node_idx);
    DecoderGraph::Node &nd = nodes[node_idx];

    map<pair<int, set<unsigned int> >, vector<unsigned int> > targets;
    for (auto ait = nd.reverse_arcs.begin(); ait != nd.reverse_arcs.end(); ++ait) {
        int target_hmm = nodes[*ait].hmm_state;
        if (target_hmm != -1)
            targets[make_pair(target_hmm, nodes[*ait].arcs)].push_back(*ait);
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

    set<unsigned int> temp_arcs = nd.reverse_arcs;
    for (auto arcit = temp_arcs.begin(); arcit != temp_arcs.end(); ++arcit)
        tie_state_suffixes(nodes, processed_nodes, stop_propagation, *arcit);
}

void
DecoderGraph::tie_word_id_suffixes(vector<DecoderGraph::Node> &nodes,
                                   bool stop_propagation,
                                   node_idx_t node_idx)
{
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);
    tie_word_id_suffixes(nodes, processed_nodes, stop_propagation, node_idx);
    prune_unreachable_nodes(nodes);
}

void
DecoderGraph::tie_word_id_suffixes_cw(vector<DecoderGraph::Node> &nodes,
                                      map<string, int> &fanout,
                                      map<string, int> &fanin,
                                      bool stop_propagation)
{
    set<node_idx_t> processed_nodes;
    set_reverse_arcs_also_from_unreachable(nodes);

    set<node_idx_t> start_nodes;
    set<node_idx_t> end_nodes;
    for (unsigned int i=0; i<nodes.size(); i++)
        if (nodes[i].flags & NODE_FAN_OUT_DUMMY)
            start_nodes.insert(i);
        else if (nodes[i].flags & NODE_FAN_IN_DUMMY)
            end_nodes.insert(i);

    for (auto enit = end_nodes.begin(); enit != end_nodes.end(); ++enit)
        tie_word_id_suffixes(nodes, processed_nodes, stop_propagation, *enit);
    prune_unreachable_nodes_cw(nodes, start_nodes, fanout, fanin);
}

void
DecoderGraph::tie_word_id_suffixes(vector<DecoderGraph::Node> &nodes,
                                   set<node_idx_t> &processed_nodes,
                                   bool stop_propagation,
                                   node_idx_t node_idx)
{
    if (processed_nodes.find(node_idx) != processed_nodes.end())
        return;
    processed_nodes.insert(node_idx);
    DecoderGraph::Node &nd = nodes[node_idx];
    set<unsigned int> temp_arcs = nd.reverse_arcs;

    map<pair<int, set<unsigned int> >, set<unsigned int> > targets;
    for (auto ait = nd.reverse_arcs.begin(); ait != nd.reverse_arcs.end(); ++ait) {
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

void
DecoderGraph::minimize_crossword_network(vector<DecoderGraph::Node> &cw_nodes,
                                         map<string, int> &fanout,
                                         map<string, int> &fanin)
{
    tie_state_prefixes_cw(cw_nodes, fanout, fanin);
    tie_word_id_prefixes_cw(cw_nodes, fanout, fanin);
    tie_state_prefixes_cw(cw_nodes, fanout, fanin);
    tie_word_id_prefixes_cw(cw_nodes, fanout, fanin);
    tie_state_suffixes_cw(cw_nodes, fanout, fanin);
    tie_word_id_suffixes_cw(cw_nodes, fanout, fanin);
    tie_state_suffixes_cw(cw_nodes, fanout, fanin);
    tie_word_id_suffixes_cw(cw_nodes, fanout, fanin);
    tie_state_suffixes_cw(cw_nodes, fanout, fanin);
    tie_word_id_suffixes_cw(cw_nodes, fanout, fanin);
    tie_state_suffixes_cw(cw_nodes, fanout, fanin);
    tie_word_id_suffixes_cw(cw_nodes, fanout, fanin);
    tie_state_prefixes_cw(cw_nodes, fanout, fanin);
    tie_word_id_prefixes_cw(cw_nodes, fanout, fanin);
}

void
DecoderGraph::print_graph(vector<DecoderGraph::Node> &nodes,
                          vector<int> path,
                          int node_idx)
{
    path.push_back(node_idx);

    if (node_idx == END_NODE) {
        for (unsigned int i = 0; i < path.size(); i++) {
            if (nodes[path[i]].hmm_state != -1)
                cout << " " << path[i] << "(" << nodes[path[i]].hmm_state << ")";
            if (nodes[path[i]].word_id != -1)
                cout << " " << path[i] << "(" << m_subwords[nodes[path[i]].word_id] << ")";
        }
        cout << endl << endl;
        return;
    }

    for (auto ait = nodes[node_idx].arcs.begin(); ait != nodes[node_idx].arcs.end(); ++ait)
        print_graph(nodes, path, *ait);
}

void
DecoderGraph::print_graph(vector<DecoderGraph::Node> &nodes)
{
    vector<int> path;
    print_graph(nodes, path, START_NODE);
}

void
DecoderGraph::print_dot_digraph(vector<DecoderGraph::Node> &nodes,
                                ostream &fstr,
                                bool mark_start_end)
{
    set<node_idx_t> node_idxs;
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
        if (mark_start_end && *it == START_NODE)
            fstr << " [label=\"start\"]" << endl;
        else if (mark_start_end && *it == END_NODE)
            fstr << " [label=\"end\"]" << endl;
        else if (nd.hmm_state != -1 && nd.word_id == -1)
            fstr << " [label=\"" << nd.hmm_state << "\"]" << endl;
        else if (nd.hmm_state == -1 && nd.word_id != -1)
            fstr << " [label=\"" << m_subwords[nd.word_id] << "\"]" << endl;
        else if (nd.hmm_state != -1 && nd.word_id != -1)
            throw string("Problem");
        //else if (nd.label.length() > 0)
        //    fstr << " [label=\"" << *it << ":" << nd.label << "\"]" << endl;
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

void
DecoderGraph::set_reverse_arcs_also_from_unreachable(vector<DecoderGraph::Node> &nodes)
{
    clear_reverse_arcs(nodes);
    for (unsigned int i = 0; i < nodes.size(); ++i) {
        DecoderGraph::Node &node = nodes[i];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            nodes[*ait].reverse_arcs.insert(i);
    }
}

void
DecoderGraph::set_reverse_arcs(vector<DecoderGraph::Node> &nodes)
{

    clear_reverse_arcs(nodes);
    set<int> processed_nodes;
    set_reverse_arcs(nodes, START_NODE, processed_nodes);
}

void
DecoderGraph::set_reverse_arcs(vector<DecoderGraph::Node> &nodes,
                               int node_idx,
                               set<int> &processed_nodes)
{
    if (processed_nodes.find(node_idx) != processed_nodes.end()) return;
    processed_nodes.insert(node_idx);

    for (auto ait = nodes[node_idx].arcs.begin(); ait != nodes[node_idx].arcs.end(); ++ait)
    {
        if (*ait == (node_idx_t)node_idx) continue;
        nodes[*ait].reverse_arcs.insert(node_idx);
        set_reverse_arcs(nodes, *ait, processed_nodes);
    }
}

void
DecoderGraph::clear_reverse_arcs(vector<DecoderGraph::Node> &nodes)
{
    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit)
        nit->reverse_arcs.clear();
}

int
DecoderGraph::merge_nodes(vector<DecoderGraph::Node> &nodes,
                          int node_idx_1,
                          int node_idx_2)
{
    if (node_idx_1 == node_idx_2)
        throw string("Merging same nodes.");

    DecoderGraph::Node &merged_node = nodes[node_idx_1];
    DecoderGraph::Node &removed_node = nodes[node_idx_2];
    removed_node.hmm_state = -1;
    removed_node.word_id = -1;

    for (auto ait = removed_node.arcs.begin(); ait != removed_node.arcs.end(); ++ait) {
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

    merged_node.to_fanout.insert(removed_node.to_fanout.begin(), removed_node.to_fanout.end());
    merged_node.from_fanin.insert(removed_node.from_fanin.begin(), removed_node.from_fanin.end());
    removed_node.to_fanout.clear();
    removed_node.from_fanin.clear();

    removed_node.arcs.clear();
    removed_node.reverse_arcs.clear();

    return node_idx_1;
}

void
DecoderGraph::connect_end_to_start_node(vector<DecoderGraph::Node> &nodes)
{
    nodes[END_NODE].arcs.insert(START_NODE);
}

void
DecoderGraph::write_graph(vector<DecoderGraph::Node> &nodes,
                          string fname)
{
    SimpleFileOutput outf(fname);
    outf << nodes.size() << "\n";
    for (unsigned int i = 0; i < nodes.size(); i++)
        outf << "n " << i << " " << nodes[i].hmm_state << " "
             << nodes[i].word_id << " " << nodes[i].arcs.size() << " "
             << nodes[i].flags << "\n";
    for (unsigned int i = 0; i < nodes.size(); i++) {
        DecoderGraph::Node &node = nodes[i];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            outf << "a " << i << " " << *ait << "\n";
    }
}

int
DecoderGraph::connect_triphone(vector<DecoderGraph::Node> &nodes,
                               string triphone,
                               node_idx_t node_idx,
                               int flag_mask)
{
    int hmm_index = m_hmm_map[triphone];
    return connect_triphone(nodes, hmm_index, node_idx, flag_mask);
}

int
DecoderGraph::connect_triphone(vector<DecoderGraph::Node> &nodes,
                               int hmm_index,
                               node_idx_t node_idx,
                               int flag_mask)
{
    Hmm &hmm = m_hmms[hmm_index];

    for (unsigned int sidx = 2; sidx < hmm.states.size(); ++sidx) {
        nodes.resize(nodes.size() + 1);
        nodes.back().hmm_state = hmm.states[sidx].model;
        nodes.back().flags |= flag_mask;
        nodes[node_idx].arcs.insert(nodes.size() - 1);
        node_idx = nodes.size() - 1;
    }

    return node_idx;
}

int
DecoderGraph::connect_triphone(vector<DecoderGraph::Node> &nodes,
                               string triphone,
                               node_idx_t node_idx,
                               map<int, string> &node_labels,
                               int flag_mask)
{
    int hmm_index = m_hmm_map[triphone];
    return connect_triphone(nodes, hmm_index, node_idx, node_labels, flag_mask);
}

int
DecoderGraph::connect_triphone(vector<DecoderGraph::Node> &nodes,
                               int hmm_index,
                               node_idx_t node_idx,
                               map<int, string> &node_labels,
                               int flag_mask)
{
    Hmm &hmm = m_hmms[hmm_index];

    for (unsigned int sidx = 2; sidx < hmm.states.size(); ++sidx) {
        nodes.resize(nodes.size() + 1);
        nodes.back().hmm_state = hmm.states[sidx].model;
        nodes.back().flags |= flag_mask;
        nodes[node_idx].arcs.insert(nodes.size() - 1);
        node_idx = nodes.size() - 1;
        string label = hmm.label + "." + to_string(sidx-2);
        node_labels[node_idx] = label;
    }

    return node_idx;
}

int
DecoderGraph::connect_word(vector<DecoderGraph::Node> &nodes,
                           string word,
                           node_idx_t node_idx,
                           int flag_mask)
{
    nodes.resize(nodes.size() + 1);
    nodes.back().word_id = m_subword_map[word];
    nodes.back().flags |= flag_mask;
    nodes[node_idx].arcs.insert(nodes.size()-1);
    return nodes.size()-1;
}

int
DecoderGraph::connect_word(vector<DecoderGraph::Node> &nodes,
                           int word_id,
                           node_idx_t node_idx,
                           int flag_mask)
{
    nodes.resize(nodes.size()+1);
    nodes.back().word_id = word_id;
    nodes.back().flags |= flag_mask;
    nodes[node_idx].arcs.insert(nodes.size()-1);
    return nodes.size()-1;
}

int
DecoderGraph::connect_dummy(vector<DecoderGraph::Node> &nodes,
                            node_idx_t node_idx,
                            int flag_mask)
{
    nodes.resize(nodes.size() + 1);
    nodes.back().flags |= flag_mask;
    nodes[node_idx].arcs.insert(nodes.size()-1);
    return nodes.size()-1;
}

void
DecoderGraph::reachable_graph_nodes(vector<DecoderGraph::Node> &nodes,
                                    set<node_idx_t> &node_idxs,
                                    node_idx_t node_idx)
{
    node_idxs.insert(node_idx);
    for (auto ait = nodes[node_idx].arcs.begin(); ait != nodes[node_idx].arcs.end(); ++ait)
        if (node_idx != *ait && node_idxs.find(*ait) == node_idxs.end())
            reachable_graph_nodes(nodes, node_idxs, *ait);
}

void
DecoderGraph::reachable_graph_nodes(vector<DecoderGraph::Node> &nodes,
                                    set<node_idx_t> &node_idxs,
                                    const set<node_idx_t> &start_nodes)
{
    for (auto snit = start_nodes.begin(); snit != start_nodes.end(); ++snit)
        reachable_graph_nodes(nodes, node_idxs, *snit);
}

int
DecoderGraph::reachable_graph_nodes(vector<DecoderGraph::Node> &nodes)
{
    set<node_idx_t> node_idxs;
    reachable_graph_nodes(nodes, node_idxs, START_NODE);
    return node_idxs.size();
}

void
DecoderGraph::prune_unreachable_nodes(vector<DecoderGraph::Node> &nodes)
{
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
        for (auto ait = old_node.arcs.begin(); ait != old_node.arcs.end(); ++ait) {
            new_node.arcs.insert(index_mapping[*ait]);
        }
    }

    nodes.swap(pruned_nodes);

    return;
}


void
DecoderGraph::prune_unreachable_nodes_cw(vector<DecoderGraph::Node> &nodes,
                                         const set<node_idx_t> &start_nodes,
                                         map<string, int> &fanout,
                                         map<string, int> &fanin)
{
    vector<DecoderGraph::Node> pruned_nodes;
    map<node_idx_t, node_idx_t> index_mapping;
    set<node_idx_t> old_node_idxs;
    reachable_graph_nodes(nodes, old_node_idxs, start_nodes);
    int new_idx = 0;
    for (auto nit = old_node_idxs.begin(); nit != old_node_idxs.end(); ++nit) {
        index_mapping[*nit] = new_idx;
        new_idx++;
    }

    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit)
        foit->second = index_mapping[foit->second];
    for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit)
        fiit->second = index_mapping[fiit->second];

    pruned_nodes.resize(old_node_idxs.size());
    for (auto nit = old_node_idxs.begin(); nit != old_node_idxs.end(); ++nit) {
        pruned_nodes[index_mapping[*nit]] = nodes[*nit];
        DecoderGraph::Node &old_node = nodes[*nit];
        DecoderGraph::Node &new_node = pruned_nodes[index_mapping[*nit]];
        new_node.arcs.clear();
        for (auto ait = old_node.arcs.begin(); ait != old_node.arcs.end(); ++ait) {
            new_node.arcs.insert(index_mapping[*ait]);
        }
    }

    nodes.swap(pruned_nodes);

    return;
}

void
DecoderGraph::add_hmm_self_transitions(vector<DecoderGraph::Node> &nodes)
{
    for (unsigned int i=0; i<nodes.size(); i++) {
        DecoderGraph::Node &node = nodes[i];
        if (node.hmm_state == -1) continue;
        node.arcs.insert(i);
    }
}

void
DecoderGraph::push_word_ids_left(vector<DecoderGraph::Node> &nodes,
                                 int &move_count,
                                 set<node_idx_t> &processed_nodes,
                                 node_idx_t node_idx,
                                 node_idx_t prev_node_idx,
                                 int subword_id)
{
    if (node_idx == START_NODE) return;
    processed_nodes.insert(node_idx);

    DecoderGraph::Node &node = nodes[node_idx];

    if (subword_id != -1 && prev_node_idx != node_idx) {
        DecoderGraph::Node &prev_node = nodes[prev_node_idx];
        swap(node.word_id, prev_node.word_id);
        swap(node.hmm_state, prev_node.hmm_state);
        move_count++;
    }

    if (node.reverse_arcs.size() == 1) subword_id = node.word_id;
    else subword_id = -1;
    if (node.flags & NODE_LM_LEFT_LIMIT) subword_id = -1;

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
DecoderGraph::push_word_ids_left(vector<DecoderGraph::Node> &nodes)
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
DecoderGraph::push_word_ids_right(vector<DecoderGraph::Node> &nodes,
                                  int &move_count,
                                  set<node_idx_t> &processed_nodes,
                                  node_idx_t node_idx,
                                  node_idx_t prev_node_idx,
                                  int subword_id)
{
    if (node_idx == END_NODE) return;
    processed_nodes.insert(node_idx);

    DecoderGraph::Node &node = nodes[node_idx];

    if (subword_id != -1 && prev_node_idx != node_idx) {
        DecoderGraph::Node &prev_node = nodes[prev_node_idx];
        swap(node.word_id, prev_node.word_id);
        swap(node.hmm_state, prev_node.hmm_state);
        move_count++;
    }

    if (node.arcs.size() == 1) subword_id = node.word_id;
    else subword_id = -1;
    if (node.flags & NODE_LM_RIGHT_LIMIT) subword_id = -1;

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
DecoderGraph::push_word_ids_right(vector<DecoderGraph::Node> &nodes)
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
DecoderGraph::num_hmm_states(vector<DecoderGraph::Node> &nodes)
{
    set<node_idx_t> node_idxs;
    reachable_graph_nodes(nodes, node_idxs, START_NODE);
    int hmm_state_count = 0;
    for (auto iit = node_idxs.begin(); iit != node_idxs.end(); ++iit)
        if (nodes[*iit].hmm_state != -1) hmm_state_count++;
    return hmm_state_count;
}

int
DecoderGraph::num_subword_states(vector<DecoderGraph::Node> &nodes)
{
    set<node_idx_t> node_idxs;
    reachable_graph_nodes(nodes, node_idxs, START_NODE);
    int subword_state_count = 0;
    for (auto iit = node_idxs.begin(); iit != node_idxs.end(); ++iit)
        if (nodes[*iit].word_id != -1) subword_state_count++;
    return subword_state_count;
}

int
DecoderGraph::num_triphones(vector<TriphoneNode> &nodes)
{
    int hmm_state_count = 0;
    for (auto nit = nodes.begin(); nit != nodes.end(); ++nit)
        if (nit->hmm_id != -1) hmm_state_count++;
    return hmm_state_count;
}

// All graph styles
void
DecoderGraph::add_long_silence(vector<DecoderGraph::Node> &nodes)
{
    nodes[END_NODE].arcs.clear();

    int ls_len = m_hmms[m_hmm_map["__"]].states.size() - 2;

    node_idx_t node_idx = END_NODE;
    node_idx = connect_word(nodes, "</s>", node_idx);
    node_idx = connect_triphone(nodes, "__", node_idx, NODE_SILENCE);
    nodes[node_idx].arcs.insert(START_NODE);
    nodes[node_idx-(ls_len-1)].flags |= NODE_DECODE_START;

    node_idx = END_NODE;
    node_idx = connect_triphone(nodes, "_", node_idx, NODE_SILENCE);
    nodes[node_idx].arcs.insert(START_NODE);
}


// Subword graph with word boundaries as below
// <s> sw sw <w> sw sw <w> sw sw sw </s>
void
DecoderGraph::add_long_silence_no_start_end_wb(vector<DecoderGraph::Node> &nodes)
{
    nodes[END_NODE].arcs.clear();

    int ls_len = m_hmms[m_hmm_map["__"]].states.size() - 2;

    node_idx_t node_idx = END_NODE;
    node_idx = connect_triphone(nodes, "__", node_idx, NODE_SILENCE);
    node_idx = connect_word(nodes, "</s>", node_idx);
    node_idx = connect_triphone(nodes, "__", node_idx, NODE_SILENCE);
    nodes[node_idx].arcs.insert(START_NODE);
    nodes[node_idx-ls_len].arcs.insert(START_NODE);
    nodes[node_idx-(ls_len-1)].flags |= NODE_DECODE_START;

    node_idx = END_NODE;
    node_idx = connect_triphone(nodes, "_", node_idx, NODE_SILENCE);
    node_idx = connect_word(nodes, "<w>", node_idx);
    nodes[node_idx].arcs.insert(START_NODE);
}


void
DecoderGraph::remove_cw_dummies(vector<DecoderGraph::Node> &nodes)
{
    set_reverse_arcs_also_from_unreachable(nodes);

    for (unsigned int i=0; i<nodes.size(); i++) {

        if (!((nodes[i].flags & NODE_FAN_OUT_DUMMY)
            || (nodes[i].flags & NODE_FAN_IN_DUMMY))) continue;

        for (auto rnit=nodes[i].reverse_arcs.begin(); rnit!=nodes[i].reverse_arcs.end(); ++rnit)
        {
            if (*rnit == i) continue;
            nodes[*rnit].arcs.insert(nodes[i].arcs.begin(), nodes[i].arcs.end());
            nodes[*rnit].arcs.erase(i);
        }
        nodes[i].arcs.clear();
    }

    clear_reverse_arcs(nodes);

    prune_unreachable_nodes(nodes);
}

