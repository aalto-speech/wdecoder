#include <cassert>
#include <ctime>
#include <algorithm>
#include <string>
#include <sstream>

#include "Lookahead.hh"

using namespace std;


void
Decoder::Lookahead::set_subword_id_la_ngram_symbol_mapping()
{
    m_subword_id_to_la_ngram_symbol.resize(decoder->m_subwords.size(), -1);
    for (unsigned int i=0; i<decoder->m_subwords.size(); i++) {
        string tmp(decoder->m_subwords[i]);
        m_subword_id_to_la_ngram_symbol[i] = m_la_lm.vocabulary_lookup[tmp];
    }
}



void
Decoder::Lookahead::find_successor_words(int node_idx,
                                         vector<int> &word_ids)
{
    set<int> successor_words;
    find_successor_words(node_idx, successor_words);
    word_ids.resize(successor_words.size());
    int i = 0;
    for (auto wit = successor_words.begin(); wit != successor_words.end(); ++wit)
        word_ids[i++] = *wit;
}


void
Decoder::Lookahead::find_successor_words(int node_idx,
                                         set<int> &word_ids,
                                         bool start_node)
{
    Decoder::Node &node = decoder->m_nodes[node_idx];

    if (!start_node && node.word_id != -1) {
        word_ids.insert(node.word_id);
        return;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        int target_node = ait->target_node;
        if (target_node == node_idx) continue;
        if (node_idx == decoder->m_long_silence_loop_start_node
                && target_node == decoder->m_long_silence_loop_end_node)
            continue;
        find_successor_words(target_node, word_ids, false);
    }
}


void
Decoder::Lookahead::get_reverse_arcs(vector<vector<Decoder::Arc> > &reverse_arcs)
{
    reverse_arcs.clear();
    reverse_arcs.resize(decoder->m_nodes.size());

    for (int ni = 0; ni < (int)(decoder->m_nodes.size()); ni++) {
        for (auto ait = decoder->m_nodes[ni].arcs.begin(); ait != decoder->m_nodes[ni].arcs.end(); ++ait) {
            if (ni == ait->target_node) continue;
            reverse_arcs[ait->target_node].resize(reverse_arcs[ait->target_node].size()+1);
            reverse_arcs[ait->target_node].back().target_node = ni;
            reverse_arcs[ait->target_node].back().update_lookahead = ait->update_lookahead;
        }
    }
}


void
Decoder::Lookahead::find_predecessor_words(int node_idx,
                                           set<int> &word_ids,
                                           const std::vector<std::vector<Decoder::Arc> > &reverse_arcs)
{
    Node &node = decoder->m_nodes[node_idx];

    if (node.word_id != -1) {
        word_ids.insert(node.word_id);
        return;
    }

    for (auto ait = reverse_arcs[node_idx].begin(); ait != reverse_arcs[node_idx].end(); ++ait) {
        int target_node = ait->target_node;
        if (target_node == node_idx) continue;
        if (node_idx == decoder->m_long_silence_loop_end_node
            && target_node == decoder->m_long_silence_loop_start_node)
            continue;
        find_predecessor_words(target_node, word_ids, reverse_arcs);
    }
}


bool
Decoder::Lookahead::detect_one_predecessor_node(int node_idx,
                                                int &predecessor_count,
                                                const std::vector<std::vector<Decoder::Arc> > &reverse_arcs)
{
    Node &node = decoder->m_nodes[node_idx];

    if (node.word_id != -1) {
        predecessor_count++;
        return true;
    }

    for (auto ait = reverse_arcs[node_idx].begin(); ait != reverse_arcs[node_idx].end(); ++ait) {
        int target_node = ait->target_node;
        if (target_node == node_idx) continue;
        if (node_idx == decoder->m_long_silence_loop_end_node
            && target_node == decoder->m_long_silence_loop_start_node)
            continue;
        detect_one_predecessor_node(target_node, predecessor_count, reverse_arcs);
        if (predecessor_count > 1) return false;
    }

    assert(predecessor_count == 1);
    return true;
}


void
Decoder::Lookahead::mark_initial_nodes(int max_depth,
                                       int curr_depth,
                                       int node_idx)
{
    Decoder::Node &node = decoder->m_nodes[node_idx];

    if (node_idx != START_NODE) {
        if (node.word_id != -1) return;
        node.flags |= NODE_INITIAL;
    }
    if (curr_depth == max_depth) return;

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (ait->target_node == node_idx) continue;
        mark_initial_nodes(max_depth, curr_depth+1, ait->target_node);
    }
}


void
Decoder::Lookahead::mark_tail_nodes(int max_depth,
                                    vector<vector<Decoder::Arc> > &reverse_arcs,
                                    int curr_depth,
                                    int node_idx)
{
    Decoder::Node &node = decoder->m_nodes[node_idx];

    if (node_idx != END_NODE) {
        node.flags |= NODE_TAIL;
        if (node.word_id != -1) return;
    }
    if (curr_depth == max_depth) return;

    for (auto ait = reverse_arcs[node_idx].begin(); ait != reverse_arcs[node_idx].end(); ++ait) {
        if (ait->target_node == node_idx) continue;
        mark_tail_nodes(max_depth, reverse_arcs, curr_depth+1, ait->target_node);
    }
}


void
NoLookahead::set_arc_la_updates()
{
    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            ait->update_lookahead = false;
    }
}


UnigramLookahead::UnigramLookahead(Decoder &decoder,
                                   string lafname)
{
    m_la_lm.read_arpa(lafname);
    this->decoder = &decoder;
    set_subword_id_la_ngram_symbol_mapping();
    set_unigram_la_scores();
    set_arc_la_updates();
}


bool descending_node_unigram_la_lp_sort(const pair<int, float> &i,
                                        const pair<int, float> &j)
{
    return (i.second > j.second);
}

int
UnigramLookahead::set_unigram_la_scores()
{
    float init_val = -1e20;
    int total_lm_nodes = 0;
    float best_lp = init_val;
    vector<pair<unsigned int, float> > sorted_nodes;
    m_la_scores.resize(decoder->m_nodes.size(), init_val);

    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.word_id == -1) continue;
        total_lm_nodes++;
        float la_prob = 0.0;
        m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[node.word_id], la_prob);
        sorted_nodes.push_back(make_pair(i, la_prob));
        best_lp = max(best_lp, la_prob);
    }

    sort(sorted_nodes.begin(), sorted_nodes.end(), descending_node_unigram_la_lp_sort);
    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);

    int score_set_count = 0;
    propagate_unigram_la_score(START_NODE, best_lp, reverse_arcs, score_set_count, true);
    m_la_scores[START_NODE] = best_lp;

    int la_state_count = 0;
    for (auto snit = sorted_nodes.begin(); snit != sorted_nodes.end(); ++snit) {
        score_set_count = 0;
        propagate_unigram_la_score(snit->first, snit->second, reverse_arcs, score_set_count, true);
        if (score_set_count > 0) la_state_count++;
    }

    for (unsigned int i=0; i<m_la_scores.size(); i++)
        if (m_la_scores[i] < -1e19) cerr << "unigram la problem in node: " << i << endl;

    return la_state_count;
}


void
UnigramLookahead::propagate_unigram_la_score(int node_idx,
                                             float score,
                                             vector<vector<Decoder::Arc> > &reverse_arcs,
                                             int &la_score_set,
                                             bool start_node)
{
    Decoder::Node &node = decoder->m_nodes[node_idx];
    if (!start_node) {
        if (m_la_scores[node_idx] > -1e19) return;
        m_la_scores[node_idx] = score;
        la_score_set++;
        if (node.word_id != -1) return;
    }

    for (auto rait = reverse_arcs[node_idx].begin(); rait != reverse_arcs[node_idx].end(); ++rait)
    {
        if (rait->target_node == node_idx) continue;
        if (rait->target_node == decoder->m_long_silence_loop_start_node && node_idx == decoder->m_long_silence_loop_end_node) continue;
        if (rait->target_node == START_NODE) continue;
        propagate_unigram_la_score(rait->target_node, score, reverse_arcs, la_score_set, false);
    }
}


float
UnigramLookahead::get_lookahead_score(int node_idx, int lm_state_idx)
{
    return m_la_scores[node_idx];
}


float
UnigramLookahead::set_arc_la_updates()
{
    float update_count = 0.0;
    float no_update_count = 0.0;
    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.flags & NODE_DECODE_START) continue;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            int j = ait->target_node;
            if (m_la_scores[i] == m_la_scores[j]) {
                ait->update_lookahead = false;
                no_update_count += 1.0;
            }
            else update_count += 1.0;
        }
    }
    return update_count / (update_count + no_update_count);
}



DummyBigramLookahead::DummyBigramLookahead(Decoder &decoder,
                                           string lafname)
{
    m_la_lm.read_arpa(lafname);
    this->decoder = &decoder;
    set_subword_id_la_ngram_symbol_mapping();
}


float
DummyBigramLookahead::get_lookahead_score(int node_idx, int word_id)
{
    vector<int> successor_words;
    find_successor_words(node_idx, successor_words);

    float dummy;
    int la_node = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[word_id], dummy);

    float la_prob = -1e20;
    for (auto swit = successor_words.begin(); swit != successor_words.end(); ++swit)
    {
        float curr_prob = 0.0;
        m_la_lm.score(la_node, m_subword_id_to_la_ngram_symbol[*swit], curr_prob);
        la_prob = max(la_prob, curr_prob);
    }

    return la_prob;
}


FullTableBigramLookahead::FullTableBigramLookahead(Decoder &decoder,
                                                   string lafname,
                                                   bool successor_lists)
{
    m_la_lm.read_arpa(lafname);
    this->decoder = &decoder;
    set_subword_id_la_ngram_symbol_mapping();

    cerr << "Setting lookahead state indices" << endl;
    m_node_la_states.resize(decoder.m_nodes.size(), -1);
    int la_state_count = set_la_state_indices_to_nodes();
    cerr << "Number of lookahead states: " << la_state_count << endl;

    if (successor_lists) {
        cerr << "Setting successor lists" << endl;
        set_la_state_successor_lists();
    }

    m_bigram_la_scores.resize(la_state_count);
    for (auto blsit = m_bigram_la_scores.begin(); blsit != m_bigram_la_scores.end(); ++blsit)
        (*blsit).resize(decoder.m_subwords.size(), -1e20);

    set_arc_la_updates();
}


int
FullTableBigramLookahead::set_la_state_indices_to_nodes()
{
    // Propagate initial la state indices
    int max_state_idx = 0;
    propagate_la_state_idx(END_NODE, 0, max_state_idx, true);

    // Find one node for each initial la state
    map<int, int> example_la_state_nodes;
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++)
        example_la_state_nodes[m_node_la_states[i]] = i;

    // Find real la states
    map<set<int>, set<int> > real_la_state_succs;
    for (auto nit = example_la_state_nodes.begin(); nit != example_la_state_nodes.end(); ++nit) {
        set<int> curr_succs;
        find_successor_words(nit->second, curr_succs, true);
        real_la_state_succs[curr_succs].insert(nit->first);
    }

    // Remap the indices
    map<int, int> la_state_remapping;
    int reidx = 0;
    for (auto ssit = real_la_state_succs.begin(); ssit != real_la_state_succs.end(); ++ssit) {
        for (auto idxit = ssit->second.begin(); idxit != ssit->second.end(); ++idxit)
            la_state_remapping[*idxit] = reidx;
        reidx++;
    }
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++)
        m_node_la_states[i] = la_state_remapping[m_node_la_states[i]];

    return real_la_state_succs.size();
}


void
FullTableBigramLookahead::propagate_la_state_idx(int node_idx,
                                                 int la_state_idx,
                                                 int &max_state_idx,
                                                 bool first_node)
{
    Decoder::Node &nd = decoder->m_nodes[node_idx];
    if (m_node_la_states[node_idx] != -1) return;

    if (!first_node && nd.word_id != -1) {
        m_node_la_states[node_idx] = ++max_state_idx;
        la_state_idx = m_node_la_states[node_idx];
    }
    else
        m_node_la_states[node_idx] = la_state_idx;

    int num_non_self_arcs = 0;
    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait)
        if (ait->target_node != node_idx) num_non_self_arcs++;
    bool la_state_change = num_non_self_arcs > 1;

    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait)
    {
        if (ait->target_node == node_idx) continue;
        if (ait->target_node == END_NODE) continue;
        if (node_idx == decoder->m_long_silence_loop_start_node && ait->target_node == decoder->m_long_silence_loop_end_node) continue;

        if (la_state_change) {
            max_state_idx++;
            propagate_la_state_idx(ait->target_node, max_state_idx, max_state_idx, false);
        }
        else
            propagate_la_state_idx(ait->target_node, la_state_idx, max_state_idx, false);
    }
}


int
FullTableBigramLookahead::set_la_state_successor_lists()
{
    int max_la_state_idx = 0;

    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {
        if (m_node_la_states[i] == -1) cerr << "warning: la state idx not set in node: " << i << endl;
        max_la_state_idx = max(max_la_state_idx, m_node_la_states[i]);
    }

    m_la_state_successor_words.resize(max_la_state_idx+1);
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {
        if (m_la_state_successor_words[m_node_la_states[i]].size() > 0) continue;
        find_successor_words(i, m_la_state_successor_words[m_node_la_states[i]]);
    }

    return m_la_state_successor_words.size();
}


float
FullTableBigramLookahead::get_lookahead_score(int node_idx, int word_id)
{
    int la_state_idx = m_node_la_states[node_idx];

    if (m_bigram_la_scores[la_state_idx][word_id] < -1e10) {
        float dummy;
        int la_node = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[word_id], dummy);
        vector<int> &word_ids = m_la_state_successor_words[la_state_idx];
        for (auto wit = word_ids.begin(); wit != word_ids.end(); ++wit) {
            float la_lm_prob = 0.0;
            m_la_lm.score(la_node, m_subword_id_to_la_ngram_symbol[*wit], la_lm_prob);
            m_bigram_la_scores[la_state_idx][word_id] = max(m_bigram_la_scores[la_state_idx][word_id], la_lm_prob);
        }
    }

    return m_bigram_la_scores[la_state_idx][word_id];
}


float
FullTableBigramLookahead::set_arc_la_updates()
{
    float update_count = 0.0;
    float no_update_count = 0.0;
    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.flags & NODE_DECODE_START) continue;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            int j = ait->target_node;
            if (m_node_la_states[i] == m_node_la_states[j]) {
                ait->update_lookahead = false;
                no_update_count += 1.0;
            }
            else update_count += 1.0;
        }
    }
    return update_count / (update_count + no_update_count);
}


PrecomputedFullTableBigramLookahead
::PrecomputedFullTableBigramLookahead(Decoder &decoder, string lafname)
    : FullTableBigramLookahead(decoder, lafname, false)
{
    set_word_id_la_states();
    set_unigram_la_scores();
    set_bigram_la_scores();
}


float
PrecomputedFullTableBigramLookahead::get_lookahead_score(int node_idx, int word_id)
{
    int la_state_idx = m_node_la_states[node_idx];
    return m_bigram_la_scores[la_state_idx][word_id];
}


void
PrecomputedFullTableBigramLookahead::set_word_id_la_states()
{
    m_word_id_la_state_lookup.resize(m_subword_id_to_la_ngram_symbol.size());
    for (int i=0; i<(int)m_word_id_la_state_lookup.size(); i++) {
        float prob;
        int nd = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[i], prob);
        m_word_id_la_state_lookup[i] = nd;
    }
}


void
PrecomputedFullTableBigramLookahead::convert_reverse_bigram_idxs(map<int, vector<int> > &reverse_bigrams)
{
    set<int> words_in_graph;
    for (int i=0; i<(int)decoder->m_nodes.size(); i++)
        if (decoder->m_nodes[i].word_id != -1)
            words_in_graph.insert(decoder->m_nodes[i].word_id);
    words_in_graph.insert(decoder->m_subword_map["<s>"]);
    words_in_graph.insert(decoder->m_subword_map["</s>"]);

    vector<int> la_ngram_symbol_to_subword_id;
    int maxidx = 0;
    for (int i=0; i<(int)m_subword_id_to_la_ngram_symbol.size(); i++)
        maxidx = max(maxidx, m_subword_id_to_la_ngram_symbol[i]);
    la_ngram_symbol_to_subword_id.resize(maxidx+1);
    for (int i=0; i<(int)m_subword_id_to_la_ngram_symbol.size(); i++)
        la_ngram_symbol_to_subword_id[m_subword_id_to_la_ngram_symbol[i]] = i;

    map<int, vector<int> > new_rev_bigrams;
    for (auto rbgit = reverse_bigrams.begin(); rbgit != reverse_bigrams.end(); ++rbgit) {
        int new_idx = la_ngram_symbol_to_subword_id[rbgit->first];
        if (words_in_graph.find(new_idx) == words_in_graph.end()) continue;
        for (int i=0; i<(int)rbgit->second.size(); i++) {
            int new_second_idx = la_ngram_symbol_to_subword_id[rbgit->second[i]];
            if (words_in_graph.find(new_second_idx) != words_in_graph.end())
                new_rev_bigrams[new_idx].push_back(new_second_idx);
        }
    }

    new_rev_bigrams.swap(reverse_bigrams);
}


void
PrecomputedFullTableBigramLookahead::find_preceeding_la_states(int node_idx,
                                                               set<int> &la_states,
                                                               const vector<vector<Decoder::Arc> > &reverse_arcs,
                                                               bool first_node,
                                                               bool state_change)
{
    if (!first_node) {
        if (state_change) {
            int la_state = m_node_la_states[node_idx];
            la_states.insert(la_state);
        }
        if (node_idx == END_NODE || decoder->m_nodes[node_idx].word_id != -1)
            return;
    }

    for (auto ait = reverse_arcs[node_idx].begin(); ait != reverse_arcs[node_idx].end(); ++ait) {
        int target_node = ait->target_node;
        if (target_node == node_idx) continue;
        if (node_idx == decoder->m_long_silence_loop_end_node
            && target_node == decoder->m_long_silence_loop_start_node)
            continue;
        find_preceeding_la_states(target_node, la_states, reverse_arcs, false, ait->update_lookahead);
    }
}


void
PrecomputedFullTableBigramLookahead::propagate_unigram_la_score(int node_idx,
                                                                float score,
                                                                int word_id,
                                                                vector<vector<Decoder::Arc> > &reverse_arcs,
                                                                vector<pair<int, float> > &unigram_la_scores,
                                                                bool start_node)
{
    Decoder::Node &node = decoder->m_nodes[node_idx];
    if (!start_node) {
        int la_state = m_node_la_states[node_idx];
        if (unigram_la_scores[la_state].second > score) return;
        unigram_la_scores[la_state].second = score;
        unigram_la_scores[la_state].first = word_id;
        if (node.word_id != -1) return;
    }

    for (auto rait = reverse_arcs[node_idx].begin(); rait != reverse_arcs[node_idx].end(); ++rait)
    {
        if (rait->target_node == node_idx) continue;
        if (rait->target_node == decoder->m_long_silence_loop_start_node && node_idx == decoder->m_long_silence_loop_end_node) continue;
        propagate_unigram_la_score(rait->target_node, score, word_id,
                                   reverse_arcs, unigram_la_scores, false);
    }
}


bool descending_node_unigram_la_lp_sort_5(const pair<int, pair<int, float> > &i,
                                          const pair<int, pair<int, float> > &j)
{
    return (i.second.second > j.second.second);
}

void
PrecomputedFullTableBigramLookahead::set_unigram_la_scores()
{
    std::vector<std::pair<int, float> > unigram_la_scores;
    unigram_la_scores.resize(decoder->m_subwords.size(), make_pair(-1,-1e20));

    // Collect all LM nodes and compute unigram la score
    vector<pair<unsigned int, pair<int, float> > > sorted_nodes;
    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.word_id == -1) continue;
        float la_prob = 0.0;
        m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[node.word_id], la_prob);
        sorted_nodes.push_back(make_pair(i, make_pair(node.word_id, la_prob)));
    }

    // Sort LM nodes in descending order and propagate unigram scores from each
    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);
    sort(sorted_nodes.begin(), sorted_nodes.end(), descending_node_unigram_la_lp_sort_5);
    for (auto snit = sorted_nodes.begin(); snit != sorted_nodes.end(); ++snit)
        propagate_unigram_la_score(snit->first, snit->second.second, snit->second.first,
                                   reverse_arcs, unigram_la_scores, true);

    // Set best unigram values for each la state/word pair
    for (int l=0; l<(int)m_bigram_la_scores.size(); l++) {
        float ug_score = unigram_la_scores[l].second;
        for (int w=0; w<(int)m_subword_id_to_la_ngram_symbol.size(); w++) {
            int la_nd = m_word_id_la_state_lookup[w];
            m_bigram_la_scores[l][w] = m_la_lm.nodes[la_nd].backoff_prob + ug_score;
        }
    }
}


void
PrecomputedFullTableBigramLookahead::set_bigram_la_scores()
{
    map<int, vector<int> > reverse_bigrams;
    m_la_lm.get_reverse_bigrams(reverse_bigrams);
    convert_reverse_bigram_idxs(reverse_bigrams);

    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);

    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {

        if (decoder->m_nodes[i].word_id == -1) continue;
        set<int> la_states;
        find_preceeding_la_states(i, la_states, reverse_arcs);

        int word_id = decoder->m_nodes[i].word_id;

        vector<int> &pred_words = reverse_bigrams[word_id];

        for (auto pwit = pred_words.begin(); pwit != pred_words.end(); ++pwit) {
            float dummy_prob = 0.0;
            int nd = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[*pwit], dummy_prob);
            float la_prob = 0.0;
            m_la_lm.score(nd, m_subword_id_to_la_ngram_symbol[word_id], la_prob);

            for (auto lasit = la_states.begin(); lasit != la_states.end(); ++lasit)
                if (la_prob > m_bigram_la_scores[*lasit][*pwit])
                    m_bigram_la_scores[*lasit][*pwit] = la_prob;
        }
    }
}


BigramScoreLookahead::BigramScoreLookahead(Decoder &decoder,
                                           string lafname)
{
    m_la_lm.read_arpa(lafname);
    this->decoder = &decoder;
    set_subword_id_la_ngram_symbol_mapping();

    m_la_scores.resize(decoder.m_nodes.size(), -1e20);
    set_bigram_la_scores_to_hmm_nodes();
    set_bigram_la_scores_to_lm_nodes();
    set_arc_la_updates();
}


void
BigramScoreLookahead::set_bigram_la_scores_to_hmm_nodes()
{
    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);

    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {

        Decoder::Node &node = decoder->m_nodes[i];
        if (node.word_id != -1) continue;

        set<int> pred_word_ids;
        find_predecessor_words(i, pred_word_ids, reverse_arcs);

        set<int> succ_word_ids;
        find_successor_words(i, succ_word_ids);

        float node_best_la_prob = -1e20;
        for (auto pwit = pred_word_ids.begin(); pwit != pred_word_ids.end(); ++pwit) {
            float dummy = 0.0;
            int lm_node = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[*pwit], dummy);

            for (auto swit = succ_word_ids.begin(); swit != succ_word_ids.end(); ++swit) {
                float la_lm_prob = 0.0;
                m_la_lm.score(lm_node, m_subword_id_to_la_ngram_symbol[*swit], la_lm_prob);
                node_best_la_prob = max(node_best_la_prob, la_lm_prob);
            }

            m_la_scores[i] = node_best_la_prob;
        }
    }
}


void
BigramScoreLookahead::set_bigram_la_scores_to_lm_nodes()
{
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.word_id != -1 && node.word_id != decoder->m_sentence_end_symbol_idx) {
            set<int> word_ids;
            find_successor_words(i, word_ids);
            float dummy = 0.0;
            int lm_node = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[node.word_id], dummy);
            m_la_scores[i] = -1e20;
            for (auto wit = word_ids.begin(); wit != word_ids.end(); ++wit) {
                float la_lm_prob = 0.0;
                m_la_lm.score(lm_node, m_subword_id_to_la_ngram_symbol[*wit], la_lm_prob);
                m_la_scores[i] = max(m_la_scores[i], la_lm_prob);
            }
        }
    }
}


float
BigramScoreLookahead::get_lookahead_score(int node_idx, int lm_state_idx)
{
    return m_la_scores[node_idx];
}


float
BigramScoreLookahead::set_arc_la_updates()
{
    float update_count = 0.0;
    float no_update_count = 0.0;
    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.flags & NODE_DECODE_START) continue;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            int j = ait->target_node;
            if (m_la_scores[i] == m_la_scores[j]) {
                ait->update_lookahead = false;
                no_update_count += 1.0;
            }
            else update_count += 1.0;
        }
    }
    return update_count / (update_count + no_update_count);
}


HybridBigramLookahead::HybridBigramLookahead(Decoder &decoder,
                                             string lafname)
{
    m_la_lm.read_arpa(lafname);
    this->decoder = &decoder;
    set_subword_id_la_ngram_symbol_mapping();

    mark_initial_nodes(1000);
    m_node_la_states.resize(decoder.m_nodes.size(), -1);
    int la_state_count = set_la_state_indices_to_nodes();
    cerr << "Number of lookahead states: " << la_state_count << endl;
    set_la_state_successor_lists();

    m_bigram_la_scores.resize(m_la_state_successor_words.size());
    for (auto blsit = m_bigram_la_scores.begin(); blsit != m_bigram_la_scores.end(); ++blsit)
        (*blsit).resize(decoder.m_subwords.size(), -1e20);

    m_bigram_la_maps.resize(decoder.m_nodes.size());
    int map_count = set_bigram_la_maps();
    cerr << "Nodes with a bigram lookahead map: " << map_count << endl;

    set_arc_la_updates();
}


int
HybridBigramLookahead::set_la_state_indices_to_nodes()
{
    // Find one node for each initial la state
    set<int> la_state_nodes;
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {
        Decoder::Node &nd = decoder->m_nodes[i];
        if ((i == START_NODE) || (i == END_NODE) || (nd.flags & NODE_SILENCE)
            || (nd.flags & NODE_CW) || (nd.flags & NODE_INITIAL)) {
            nd.flags |= NODE_BIGRAM_LA_TABLE;
            la_state_nodes.insert(i);
        }
    }
    cerr << "Number of nodes with a lookahead table: " << la_state_nodes.size() << endl;

    // Find real la states
    map<set<int>, set<int> > real_la_state_succs;
    for (auto nit = la_state_nodes.begin(); nit != la_state_nodes.end(); ++nit) {
        set<int> curr_succs;
        find_successor_words(*nit, curr_succs, true);
        real_la_state_succs[curr_succs].insert(*nit);
    }

    map<int, int> la_state_mapping;
    int laidx = 0;
    for (auto ssit = real_la_state_succs.begin(); ssit != real_la_state_succs.end(); ++ssit) {
        for (auto idxit = ssit->second.begin(); idxit != ssit->second.end(); ++idxit)
            la_state_mapping[*idxit] = laidx;
        laidx++;
    }

    for (auto smit=la_state_mapping.begin(); smit != la_state_mapping.end(); ++smit)
        m_node_la_states[smit->first] = smit->second;

    return real_la_state_succs.size();
}


int
HybridBigramLookahead::set_la_state_successor_lists()
{
    int max_la_state_idx = 0;
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++)
    {
        int la_state = m_node_la_states[i];
        if (la_state == -1) continue;
        max_la_state_idx = max(max_la_state_idx, la_state);
    }

    m_la_state_successor_words.resize(max_la_state_idx+1);
    for (unsigned int i=0; i<decoder->m_nodes.size(); ++i) {
        int la_state = m_node_la_states[i];
        if (la_state == -1) continue;
        if (m_la_state_successor_words[la_state].size() > 0) continue;
        find_successor_words(i, m_la_state_successor_words[la_state]);
    }

    return m_la_state_successor_words.size();
}


int
HybridBigramLookahead::set_bigram_la_maps()
{

    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);

    int map_count = 0;
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {

        Decoder::Node &node = decoder->m_nodes[i];
        if (node.flags & NODE_BIGRAM_LA_TABLE) continue;

        set<int> predecessor_words;
        find_predecessor_words(i, predecessor_words, reverse_arcs);

        set<int> successor_words;
        find_successor_words(i, successor_words);

        map<int, float> &bigram_la_map = m_bigram_la_maps[i];
        for (auto pwit = predecessor_words.begin(); pwit != predecessor_words.end(); ++pwit) {
            float dummy = 0.0;
            int lm_node = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[*pwit], dummy);
            bigram_la_map[*pwit] = -1e20;
            for (auto swit = successor_words.begin(); swit != successor_words.end(); ++swit) {
                float la_lm_prob = 0.0;
                m_la_lm.score(lm_node, m_subword_id_to_la_ngram_symbol[*swit], la_lm_prob);
                bigram_la_map[*pwit] = max(bigram_la_map[*pwit], la_lm_prob);
            }
        }

        map_count++;
    }

    return map_count;
}


float
HybridBigramLookahead::get_lookahead_score(int node_idx, int word_id)
{
    if (decoder->m_nodes[node_idx].flags & NODE_BIGRAM_LA_TABLE) {
        int la_state_idx = m_node_la_states[node_idx];
        float &score = m_bigram_la_scores[la_state_idx][word_id];
        if (score < -1e10) {
            float dummy;
            int la_node = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[word_id], dummy);
            vector<int> &word_ids = m_la_state_successor_words[la_state_idx];
            for (auto wit = word_ids.begin(); wit != word_ids.end(); ++wit) {
                float la_lm_prob = 0.0;
                m_la_lm.score(la_node, m_subword_id_to_la_ngram_symbol[*wit], la_lm_prob);
                score = max(score, la_lm_prob);
            }
        }
        return score;
    }
    else {
        //if (m_bigram_la_maps[node_idx].find(word_id) == m_bigram_la_maps[node_idx].end())
        //    cerr << "la problem in node: " << node_idx << endl;
        return m_bigram_la_maps[node_idx][word_id];
    }
}


float
HybridBigramLookahead::set_arc_la_updates()
{
    float update_count = 0.0;
    float no_update_count = 0.0;
    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.flags & NODE_DECODE_START) continue;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            int j = ait->target_node;

            if (m_bigram_la_maps[i].size() > 0 &&
                m_bigram_la_maps[j].size() > 0 &&
                m_bigram_la_maps[i] == m_bigram_la_maps[j])
            {
                ait->update_lookahead = false;
                no_update_count += 1.0;
            }
            else if (m_node_la_states[i] != -1 &&
                     m_node_la_states[j] != -1 &&
                     m_node_la_states[i] == m_node_la_states[j])
            {
                ait->update_lookahead = false;
                no_update_count += 1.0;
            }
            else update_count += 1.0;
        }
    }
    return update_count / (update_count + no_update_count);
}


PrecomputedHybridBigramLookahead
::PrecomputedHybridBigramLookahead(Decoder &decoder, string lafname)
    : HybridBigramLookahead(decoder, lafname)
{
    m_la_state_successor_words.clear();
    set_word_id_la_states();
    set_unigram_la_scores();
    set_bigram_la_scores();
}


float
PrecomputedHybridBigramLookahead::get_lookahead_score(int node_idx, int word_id)
{
    if (decoder->m_nodes[node_idx].flags & NODE_BIGRAM_LA_TABLE) {
        int la_state_idx = m_node_la_states[node_idx];
        return m_bigram_la_scores[la_state_idx][word_id];
    }
    else
        return m_bigram_la_maps[node_idx][word_id];
}


void
PrecomputedHybridBigramLookahead::set_word_id_la_states()
{
    m_word_id_la_state_lookup.resize(m_subword_id_to_la_ngram_symbol.size());
    for (int i=0; i<(int)m_word_id_la_state_lookup.size(); i++) {
        float prob;
        int nd = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[i], prob);
        m_word_id_la_state_lookup[i] = nd;
    }
}


void
PrecomputedHybridBigramLookahead::convert_reverse_bigram_idxs(map<int, vector<int> > &reverse_bigrams)
{
    set<int> words_in_graph;
    for (int i=0; i<(int)decoder->m_nodes.size(); i++)
        if (decoder->m_nodes[i].word_id != -1)
            words_in_graph.insert(decoder->m_nodes[i].word_id);
    words_in_graph.insert(decoder->m_subword_map["<s>"]);
    words_in_graph.insert(decoder->m_subword_map["</s>"]);

    vector<int> la_ngram_symbol_to_subword_id;
    int maxidx = 0;
    for (int i=0; i<(int)m_subword_id_to_la_ngram_symbol.size(); i++)
        maxidx = max(maxidx, m_subword_id_to_la_ngram_symbol[i]);
    la_ngram_symbol_to_subword_id.resize(maxidx+1);
    for (int i=0; i<(int)m_subword_id_to_la_ngram_symbol.size(); i++)
        la_ngram_symbol_to_subword_id[m_subword_id_to_la_ngram_symbol[i]] = i;

    map<int, vector<int> > new_rev_bigrams;
    for (auto rbgit = reverse_bigrams.begin(); rbgit != reverse_bigrams.end(); ++rbgit) {
        int new_idx = la_ngram_symbol_to_subword_id[rbgit->first];
        if (words_in_graph.find(new_idx) == words_in_graph.end()) continue;
        for (int i=0; i<(int)rbgit->second.size(); i++) {
            int new_second_idx = la_ngram_symbol_to_subword_id[rbgit->second[i]];
            if (words_in_graph.find(new_second_idx) != words_in_graph.end())
                new_rev_bigrams[new_idx].push_back(new_second_idx);
        }
    }

    new_rev_bigrams.swap(reverse_bigrams);
}


void
PrecomputedHybridBigramLookahead::find_preceeding_la_states(int node_idx,
                                                            set<int> &la_states,
                                                            const vector<vector<Decoder::Arc> > &reverse_arcs,
                                                            bool first_node,
                                                            bool state_change)
{
    Decoder::Node &node = decoder->m_nodes[node_idx];

    if (!first_node) {
        if (!(node.flags & NODE_BIGRAM_LA_TABLE)) return;
        if (state_change) {
            int la_state = m_node_la_states[node_idx];
            la_states.insert(la_state);
        }
        if (node.word_id != -1) return;
    }

    for (auto ait = reverse_arcs[node_idx].begin(); ait != reverse_arcs[node_idx].end(); ++ait) {
        int target_node = ait->target_node;
        if (target_node == node_idx) continue;
        if (node_idx == decoder->m_long_silence_loop_end_node
            && target_node == decoder->m_long_silence_loop_start_node)
            continue;
        find_preceeding_la_states(target_node, la_states, reverse_arcs, false, ait->update_lookahead);
    }
}


void
PrecomputedHybridBigramLookahead::find_first_lm_nodes(set<int> &lm_nodes,
                                                      int node_idx)
{
    if (decoder->m_nodes[node_idx].word_id != -1) {
        lm_nodes.insert(node_idx);
        return;
    }

    Decoder::Node &node = decoder->m_nodes[node_idx];
    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        int target_node = ait->target_node;
        if (target_node == node_idx) continue;
        find_first_lm_nodes(lm_nodes, target_node);
    }
}


void
PrecomputedHybridBigramLookahead::find_cw_lm_nodes(set<int> &lm_nodes)
{
    for (int i=0; i<decoder->m_nodes.size(); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if ((node.flags & NODE_CW) && node.word_id != -1)
            lm_nodes.insert(i);
    }
}


void
PrecomputedHybridBigramLookahead::find_sentence_end_lm_node(set<int> &lm_nodes)
{
    int sentence_end_word_id = decoder->m_subword_map["</s>"];

    for (int i=0; i<decoder->m_nodes.size(); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.word_id == sentence_end_word_id)
            lm_nodes.insert(i);
    }
}


void
PrecomputedHybridBigramLookahead::propagate_unigram_la_score(int node_idx,
                                                             float score,
                                                             int word_id,
                                                             vector<vector<Decoder::Arc> > &reverse_arcs,
                                                             vector<pair<int, float> > &unigram_la_scores,
                                                             bool start_node)
{
    Decoder::Node &node = decoder->m_nodes[node_idx];

    if (!start_node) {
        if (!(node.flags & NODE_BIGRAM_LA_TABLE)) return;
        int la_state = m_node_la_states[node_idx];
        if (unigram_la_scores[la_state].second > score) return;
        unigram_la_scores[la_state].second = score;
        unigram_la_scores[la_state].first = word_id;
        if (node.word_id != -1) return;
    }

    for (auto rait = reverse_arcs[node_idx].begin(); rait != reverse_arcs[node_idx].end(); ++rait)
    {
        if (rait->target_node == node_idx) continue;
        if (rait->target_node == decoder->m_long_silence_loop_start_node && node_idx == decoder->m_long_silence_loop_end_node) continue;
        propagate_unigram_la_score(rait->target_node, score, word_id,
                                   reverse_arcs, unigram_la_scores, false);
    }
}


void
PrecomputedHybridBigramLookahead::set_unigram_la_scores()
{
    vector<pair<int, float> > unigram_la_scores;
    unigram_la_scores.resize(decoder->m_subwords.size(), make_pair(-1,-1e20));

    // Collect all LM nodes and compute unigram la score
    vector<pair<unsigned int, pair<int, float> > > sorted_nodes;
    set<int> lm_nodes;
    find_first_lm_nodes(lm_nodes);
    find_cw_lm_nodes(lm_nodes);
    find_sentence_end_lm_node(lm_nodes);

    for (auto nit=lm_nodes.begin(); nit != lm_nodes.end(); ++nit) {
        Decoder::Node &node = decoder->m_nodes[*nit];
        float la_prob = 0.0;
        m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[node.word_id], la_prob);
        sorted_nodes.push_back(make_pair(*nit, make_pair(node.word_id, la_prob)));
    }

    // Sort LM nodes in descending order and propagate unigram scores from each
    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);
    sort(sorted_nodes.begin(), sorted_nodes.end(), descending_node_unigram_la_lp_sort_5);
    for (auto snit = sorted_nodes.begin(); snit != sorted_nodes.end(); ++snit)
        propagate_unigram_la_score(snit->first, snit->second.second, snit->second.first,
                                   reverse_arcs, unigram_la_scores, true);

    // Set best unigram values for each la state/predecessor word pair
    for (int l=0; l<(int)m_bigram_la_scores.size(); l++) {
        float ug_score = unigram_la_scores[l].second;
        for (int w=0; w<(int)m_subword_id_to_la_ngram_symbol.size(); w++) {
            int la_nd = m_word_id_la_state_lookup[w];
            m_bigram_la_scores[l][w] = m_la_lm.nodes[la_nd].backoff_prob + ug_score;
        }
    }
}


void
PrecomputedHybridBigramLookahead::set_bigram_la_scores()
{
    map<int, vector<int> > reverse_bigrams;
    m_la_lm.get_reverse_bigrams(reverse_bigrams);
    convert_reverse_bigram_idxs(reverse_bigrams);

    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);

    set<int> lm_nodes;
    find_first_lm_nodes(lm_nodes);
    find_cw_lm_nodes(lm_nodes);
    find_sentence_end_lm_node(lm_nodes);

    for (auto nit=lm_nodes.begin(); nit != lm_nodes.end(); ++nit) {

        if (decoder->m_nodes[*nit].word_id == -1) continue;
        set<int> la_states;
        find_preceeding_la_states(*nit, la_states, reverse_arcs);

        int word_id = decoder->m_nodes[*nit].word_id;

        vector<int> &pred_words = reverse_bigrams[word_id];

        for (auto pwit = pred_words.begin(); pwit != pred_words.end(); ++pwit) {
            float dummy_prob = 0.0;
            int nd = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[*pwit], dummy_prob);
            float la_prob = 0.0;
            m_la_lm.score(nd, m_subword_id_to_la_ngram_symbol[word_id], la_prob);

            for (auto lasit = la_states.begin(); lasit != la_states.end(); ++lasit)
                if (la_prob > m_bigram_la_scores[*lasit][*pwit])
                    m_bigram_la_scores[*lasit][*pwit] = la_prob;
        }
    }
}


FullTableBigramLookahead2::FullTableBigramLookahead2(Decoder &decoder,
                                                     string lafname)
{
    m_la_lm.read_arpa(lafname);
    this->decoder = &decoder;
    set_subword_id_la_ngram_symbol_mapping();

    m_one_predecessor_la_scores.resize(decoder.m_nodes.size(), -1e20);
    int count = set_one_predecessor_la_scores();
    cerr << "Number of nodes with one predecessor: " << count << endl;

    cerr << "Setting lookahead state indices and successor lists" << endl;
    m_node_la_states.resize(decoder.m_nodes.size(), -1);
    int la_state_count = set_la_state_indices_to_nodes();
    set_la_state_successor_lists();
    cerr << "Number of lookahead states: " << la_state_count << endl;

    assert((int)m_la_state_successor_words.size() == la_state_count);
    m_bigram_la_scores.resize(m_la_state_successor_words.size());
    for (auto blsit = m_bigram_la_scores.begin(); blsit != m_bigram_la_scores.end(); ++blsit)
        (*blsit).resize(decoder.m_subwords.size(), -1e20);

    set_arc_la_updates();
}


int
FullTableBigramLookahead2::set_one_predecessor_la_scores()
{
    int one_predecessor_nodes = 0;

    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);

    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {

        int tmp = 0;
        bool one_predecessor = detect_one_predecessor_node(i, tmp, reverse_arcs);
        if (!one_predecessor) continue;

        set<int> pred_word_ids;
        find_predecessor_words(i, pred_word_ids, reverse_arcs);
        assert(pred_word_ids.size() == 1);
        int pred_word_id = (*(pred_word_ids.begin()));

        set<int> word_ids;
        find_successor_words(i, word_ids);
        float dummy = 0.0;
        int lm_node = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[pred_word_id], dummy);
        m_one_predecessor_la_scores[i] = -1e20;
        for (auto wit = word_ids.begin(); wit != word_ids.end(); ++wit) {
            float la_lm_prob = 0.0;
            m_la_lm.score(lm_node, m_subword_id_to_la_ngram_symbol[*wit], la_lm_prob);
            m_one_predecessor_la_scores[i] = max(m_one_predecessor_la_scores[i], la_lm_prob);
        }

        one_predecessor_nodes++;
    }

    return one_predecessor_nodes;
}


int
FullTableBigramLookahead2::set_la_state_indices_to_nodes()
{
    // Find one node for each initial la state
    set<int> la_state_nodes;
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {
        if (m_one_predecessor_la_scores[i] > -1e10) continue;
        la_state_nodes.insert(i);
    }
    cerr << "Number of nodes with a lookahead table: " << la_state_nodes.size() << endl;

    // Find real la states
    map<set<int>, set<int> > real_la_state_succs;
    for (auto nit = la_state_nodes.begin(); nit != la_state_nodes.end(); ++nit) {
        set<int> curr_succs;
        find_successor_words(*nit, curr_succs, true);
        real_la_state_succs[curr_succs].insert(*nit);
    }

    map<int, int> la_state_mapping;
    int laidx = 0;
    for (auto ssit = real_la_state_succs.begin(); ssit != real_la_state_succs.end(); ++ssit) {
        for (auto idxit = ssit->second.begin(); idxit != ssit->second.end(); ++idxit)
            la_state_mapping[*idxit] = laidx;
        laidx++;
    }

    for (auto smit=la_state_mapping.begin(); smit != la_state_mapping.end(); ++smit)
        m_node_la_states[smit->first] = smit->second;

    return real_la_state_succs.size();
}


int
FullTableBigramLookahead2::set_la_state_successor_lists()
{
    int max_la_state_idx = 0;

    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {
        if (m_node_la_states[i] == -1) continue;
        max_la_state_idx = max(max_la_state_idx, m_node_la_states[i]);
    }

    m_la_state_successor_words.resize(max_la_state_idx+1);
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {
        if (m_node_la_states[i] == -1) continue;
        if (m_la_state_successor_words[m_node_la_states[i]].size() > 0) continue;
        find_successor_words(i, m_la_state_successor_words[m_node_la_states[i]]);
    }

    return m_la_state_successor_words.size();
}


float
FullTableBigramLookahead2::get_lookahead_score(int node_idx, int word_id)
{
    if (m_one_predecessor_la_scores[node_idx] > -1e10)
        return m_one_predecessor_la_scores[node_idx];

    int la_state_idx = m_node_la_states[node_idx];
    float &score = m_bigram_la_scores[la_state_idx][word_id];

    if (score < -1e10) {
        float dummy;
        int la_node = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[word_id], dummy);
        vector<int> &word_ids = m_la_state_successor_words[la_state_idx];
        for (auto wit = word_ids.begin(); wit != word_ids.end(); ++wit) {
            float la_lm_prob = 0.0;
            m_la_lm.score(la_node, m_subword_id_to_la_ngram_symbol[*wit], la_lm_prob);
            score = max(score, la_lm_prob);
        }
    }

    return score;
}


float
FullTableBigramLookahead2::set_arc_la_updates()
{
    float update_count = 0.0;
    float no_update_count = 0.0;
    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.flags & NODE_DECODE_START) continue;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            int j = ait->target_node;

            if (m_one_predecessor_la_scores[i] > -1e10 &&
                m_one_predecessor_la_scores[j] > -1e10 &&
                m_one_predecessor_la_scores[i] == m_one_predecessor_la_scores[j])
            {
                ait->update_lookahead = false;
                no_update_count += 1.0;
            }
            else if (m_node_la_states[i] != -1 &&
                     m_node_la_states[j] != -1 &&
                     m_node_la_states[i] == m_node_la_states[j])
            {
                ait->update_lookahead = false;
                no_update_count += 1.0;
            }
            else update_count += 1.0;
        }
    }
    return update_count / (update_count + no_update_count);
}


LargeBigramLookahead::LargeBigramLookahead(Decoder &decoder,
                                           string lafname)
{
    m_la_lm.read_arpa(lafname);
    this->decoder = &decoder;
    set_subword_id_la_ngram_symbol_mapping();

    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);
    mark_initial_nodes(1000);
    mark_tail_nodes(1000, reverse_arcs);

    int tail_count = 0;
    int initial_count = 0;
    for (int i=0; i<(int)decoder.m_nodes.size(); i++) {
        if (decoder.m_nodes[i].flags & NODE_TAIL) tail_count++;
        if (decoder.m_nodes[i].flags & NODE_INITIAL) initial_count++;
    }
    cerr << "Tail nodes: " << tail_count << endl;
    cerr << "Initial nodes: " << initial_count << endl;

    m_node_la_states.resize(decoder.m_nodes.size(), -1);
    int la_count = set_la_state_indices_to_nodes();
    m_lookahead_states.resize(la_count);
    cerr << "Number of lookahead states: " << la_count << endl;

    set_word_id_la_states();

    set_arc_la_updates();

    cerr << "Propagating unigram scores" << endl;
    set_unigram_la_scores();

    cerr << "Propagating bigram scores" << endl;
    time_t rawtime;
    time(&rawtime);
    cerr << "time: " << ctime(&rawtime);

    int bigram_score_count = set_bigram_la_scores_2();

    time(&rawtime);
    cerr << "time: " << ctime(&rawtime);

    cerr << "Bigram scores set: " << bigram_score_count << endl;
    int bigram_score_la_state_count = 0;
    for (unsigned int i=0; i<m_lookahead_states.size(); i++)
        if (m_lookahead_states[i].m_scores.size() > 0)
            bigram_score_la_state_count++;
    cerr << "La states with bigram scores: " << bigram_score_la_state_count << "/" << m_lookahead_states.size() << endl;
}


void
LargeBigramLookahead::set_word_id_la_states()
{
    m_word_id_la_state_lookup.resize(m_subword_id_to_la_ngram_symbol.size());
    for (int i=0; i<(int)m_word_id_la_state_lookup.size(); i++) {
        float prob;
        int nd = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[i], prob);
        m_word_id_la_state_lookup[i] = nd;
    }
}


int
LargeBigramLookahead::set_la_state_indices_to_nodes()
{
    // Propagate initial la state indices
    int max_state_idx = 0;
    propagate_la_state_idx(END_NODE, 0, max_state_idx, true);

    // Remap indices
    // Set tail nodes to the same la state as the end node
    map<int, int> remap;
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {
        if (decoder->m_nodes[i].flags & NODE_TAIL)
            m_node_la_states[i] = m_node_la_states[END_NODE];
        remap[m_node_la_states[i]] = 0;
    }

    int idx = 0;
    for (auto rmit = remap.begin(); rmit != remap.end(); ++rmit) {
        rmit->second = idx;
        idx++;
    }

    for (unsigned int i=0; i<decoder->m_nodes.size(); i++)
        m_node_la_states[i] = remap[m_node_la_states[i]];

    return remap.size();
}


float
LargeBigramLookahead::get_lookahead_score(int node_idx, int word_id)
{
    int la_state = m_node_la_states[node_idx];
    LookaheadState &state = m_lookahead_states[la_state];

    if (state.m_scores.find(word_id) != state.m_scores.end())
        return state.m_scores[word_id];
    else {
        /*
        float prob = 0.0;
        int nd = m_word_id_la_state_lookup[word_id];
        m_la_lm.score(nd, m_subword_id_to_la_ngram_symbol[state.m_best_unigram_word_id], prob);
        return prob;
        */

        // OPTION 2: takes slightly more memory, faster
        int nd2 = m_word_id_la_state_lookup[word_id];
        float prob2 = m_la_lm.nodes[nd2].backoff_prob + state.m_best_unigram_score;
        return prob2;
    }
}


float
LargeBigramLookahead::set_arc_la_updates()
{
    float update_count = 0.0;
    float no_update_count = 0.0;
    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.flags & NODE_DECODE_START) continue;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            int j = ait->target_node;

            if (m_node_la_states[i] != -1 &&
                m_node_la_states[j] != -1 &&
                m_node_la_states[i] == m_node_la_states[j])
            {
                ait->update_lookahead = false;
                no_update_count += 1.0;
            }
            else update_count += 1.0;
        }
    }
    return update_count / (update_count + no_update_count);
}


void
LargeBigramLookahead::propagate_la_state_idx(int node_idx,
                                             int la_state_idx,
                                             int &max_state_idx,
                                             bool first_node)
{
    Decoder::Node &nd = decoder->m_nodes[node_idx];
    if (m_node_la_states[node_idx] != -1) return;

    if (!first_node && nd.word_id != -1) {
        m_node_la_states[node_idx] = ++max_state_idx;
        la_state_idx = m_node_la_states[node_idx];
    }
    else
        m_node_la_states[node_idx] = la_state_idx;

    int num_non_self_arcs = 0;
    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait)
        if (ait->target_node != node_idx) num_non_self_arcs++;
    bool la_state_change = num_non_self_arcs > 1;

    for (auto ait = nd.arcs.begin(); ait != nd.arcs.end(); ++ait)
    {
        if (ait->target_node == node_idx) continue;
        if (ait->target_node == END_NODE) continue;
        if (node_idx == decoder->m_long_silence_loop_start_node && ait->target_node == decoder->m_long_silence_loop_end_node) continue;

        if (la_state_change) {
            max_state_idx++;
            propagate_la_state_idx(ait->target_node, max_state_idx, max_state_idx, false);
        }
        else
            propagate_la_state_idx(ait->target_node, la_state_idx, max_state_idx, false);
    }
}


void
LargeBigramLookahead::find_preceeding_la_states(int node_idx,
                                                set<int> &la_states,
                                                const vector<vector<Decoder::Arc> > &reverse_arcs,
                                                bool first_node,
                                                bool state_change)
{
    if (!first_node) {
        if (state_change) {
            int la_state = m_node_la_states[node_idx];
            la_states.insert(la_state);
        }
        if (node_idx == END_NODE || decoder->m_nodes[node_idx].word_id != -1)
            return;
    }

    for (auto ait = reverse_arcs[node_idx].begin(); ait != reverse_arcs[node_idx].end(); ++ait) {
        int target_node = ait->target_node;
        if (target_node == node_idx) continue;
        if (node_idx == decoder->m_long_silence_loop_end_node
            && target_node == decoder->m_long_silence_loop_start_node)
            continue;
        find_preceeding_la_states(target_node, la_states, reverse_arcs, false, ait->update_lookahead);
    }
}


void
LargeBigramLookahead::convert_reverse_bigram_idxs(map<int, vector<int> > &reverse_bigrams)
{
    set<int> words_in_graph;
    for (int i=0; i<(int)decoder->m_nodes.size(); i++)
        if (decoder->m_nodes[i].word_id != -1)
            words_in_graph.insert(decoder->m_nodes[i].word_id);
    words_in_graph.insert(decoder->m_subword_map["<s>"]);
    words_in_graph.insert(decoder->m_subword_map["</s>"]);

    vector<int> la_ngram_symbol_to_subword_id;
    int maxidx = 0;
    for (int i=0; i<(int)m_subword_id_to_la_ngram_symbol.size(); i++)
        maxidx = max(maxidx, m_subword_id_to_la_ngram_symbol[i]);
    la_ngram_symbol_to_subword_id.resize(maxidx+1);
    for (int i=0; i<(int)m_subword_id_to_la_ngram_symbol.size(); i++)
        la_ngram_symbol_to_subword_id[m_subword_id_to_la_ngram_symbol[i]] = i;

    map<int, vector<int> > new_rev_bigrams;
    for (auto rbgit = reverse_bigrams.begin(); rbgit != reverse_bigrams.end(); ++rbgit) {
        int new_idx = la_ngram_symbol_to_subword_id[rbgit->first];
        if (words_in_graph.find(new_idx) == words_in_graph.end()) continue;
        for (int i=0; i<(int)rbgit->second.size(); i++) {
            int new_second_idx = la_ngram_symbol_to_subword_id[rbgit->second[i]];
            if (words_in_graph.find(new_second_idx) != words_in_graph.end())
                new_rev_bigrams[new_idx].push_back(new_second_idx);
        }
    }

    new_rev_bigrams.swap(reverse_bigrams);
}


void
LargeBigramLookahead::propagate_unigram_la_score(int node_idx,
                                                 float score,
                                                 int word_id,
                                                 vector<vector<Decoder::Arc> > &reverse_arcs,
                                                 bool start_node)
{
    Decoder::Node &node = decoder->m_nodes[node_idx];
    if (!start_node) {
        LookaheadState &la_state = m_lookahead_states[m_node_la_states[node_idx]];
        if (la_state.m_best_unigram_score > score) return;
        la_state.m_best_unigram_score = score;
        la_state.m_best_unigram_word_id = word_id;
        if (node.word_id != -1) return;
    }

    for (auto rait = reverse_arcs[node_idx].begin(); rait != reverse_arcs[node_idx].end(); ++rait)
    {
        if (rait->target_node == node_idx) continue;
        if (rait->target_node == decoder->m_long_silence_loop_start_node && node_idx == decoder->m_long_silence_loop_end_node) continue;
        propagate_unigram_la_score(rait->target_node, score, word_id, reverse_arcs, false);
    }
}


bool descending_node_unigram_la_lp_sort_2(const pair<int, pair<int, float> > &i,
                                          const pair<int, pair<int, float> > &j)
{
    return (i.second.second > j.second.second);
}

void
LargeBigramLookahead::set_unigram_la_scores()
{
    vector<pair<unsigned int, pair<int, float> > > sorted_nodes;
    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];
        if (node.word_id == -1) continue;
        float la_prob = 0.0;
        m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[node.word_id], la_prob);
        sorted_nodes.push_back(make_pair(i, make_pair(node.word_id, la_prob)));
    }

    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);
    sort(sorted_nodes.begin(), sorted_nodes.end(), descending_node_unigram_la_lp_sort_2);
    for (auto snit = sorted_nodes.begin(); snit != sorted_nodes.end(); ++snit)
        propagate_unigram_la_score(snit->first, snit->second.second, snit->second.first,
                                   reverse_arcs, true);
}


int
LargeBigramLookahead::set_bigram_la_scores()
{
    map<int, vector<int> > reverse_bigrams;
    m_la_lm.get_reverse_bigrams(reverse_bigrams);
    convert_reverse_bigram_idxs(reverse_bigrams);

    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);

    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {

        if (i % 10000 == 0) {
            int bsc = 0;
            for (auto lasit=m_lookahead_states.begin(); lasit != m_lookahead_states.end(); ++lasit)
                bsc += lasit->m_scores.size();
            cerr << "node " << i << " / " << decoder->m_nodes.size()
                 << ", bigram scores: " << bsc << endl;
        }

        if (decoder->m_nodes[i].word_id == -1) continue;
        int word_id = decoder->m_nodes[i].word_id;
        vector<int> pred_words = reverse_bigrams[word_id];
        set<int> processed_la_states;
        propagate_bigram_la_scores(i, word_id, pred_words, reverse_arcs,
                                   processed_la_states, true, false);
    }

    int bigram_score_count = 0;
    for (auto lasit=m_lookahead_states.begin(); lasit != m_lookahead_states.end(); ++lasit)
        bigram_score_count += lasit->m_scores.size();

    return bigram_score_count;
}


void
LargeBigramLookahead::propagate_bigram_la_scores(int node_idx,
                                                 int word_id,
                                                 vector<int> predecessor_words,
                                                 vector<vector<Decoder::Arc> > &reverse_arcs,
                                                 set<int> &processed_la_states,
                                                 bool start_node,
                                                 bool la_state_change)
{
    int la_state = m_node_la_states[node_idx];
    if (!start_node && la_state_change)
//        && processed_la_states.find(la_state) == processed_la_states.end())
    {
        int best_unigram_word_id = m_lookahead_states[la_state].m_best_unigram_word_id;

        for (auto pwit = predecessor_words.begin(); pwit != predecessor_words.end(); )
        {
            if (word_id == best_unigram_word_id) {
                ++pwit;
                continue;
            }

            float la_prob;
            int nd = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[*pwit], la_prob);
            la_prob = 0.0;
            m_la_lm.score(nd, m_subword_id_to_la_ngram_symbol[word_id], la_prob);

            float unigram_prob = 0.0;
            m_la_lm.score(nd, m_subword_id_to_la_ngram_symbol[best_unigram_word_id], unigram_prob);

            if (unigram_prob > la_prob) {
                pwit = predecessor_words.erase(pwit);
            }
            else {
                map<int, float> &la_scores = m_lookahead_states[la_state].m_scores;
                if (la_scores.find(*pwit) == la_scores.end()) {
                    la_scores[*pwit] = la_prob;
                    ++pwit;
                }
                else {
                    if (la_scores[*pwit] > la_prob) {
                        pwit = predecessor_words.erase(pwit);
                    }
                    else {
                        la_scores[*pwit] = la_prob;
                        ++pwit;
                    }
                }
            }
        }

        //processed_la_states.insert(la_state);
        if (predecessor_words.size() == 0) return;
    }

    if (!start_node && decoder->m_nodes[node_idx].word_id != -1) return;

    for (auto rait = reverse_arcs[node_idx].begin(); rait != reverse_arcs[node_idx].end(); ++rait)
    {
        if (rait->target_node == node_idx) continue;
        if (rait->target_node == decoder->m_long_silence_loop_start_node && node_idx == decoder->m_long_silence_loop_end_node) continue;
        propagate_bigram_la_scores(rait->target_node, word_id, predecessor_words,
                                   reverse_arcs, processed_la_states, false, rait->update_lookahead);
    }
}


int
LargeBigramLookahead::set_bigram_la_scores_2()
{
    int bigram_score_count = 0;

    map<int, vector<int> > reverse_bigrams;
    m_la_lm.get_reverse_bigrams(reverse_bigrams);
    convert_reverse_bigram_idxs(reverse_bigrams);

    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);

    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {

        if (decoder->m_nodes.size() > 50000 && i % 10000 == 0) {
            int bsc = 0;
            for (auto lasit=m_lookahead_states.begin(); lasit != m_lookahead_states.end(); ++lasit)
                bsc += lasit->m_scores.size();
            cerr << "node " << i << " / " << decoder->m_nodes.size()
                 << ", bigram scores: " << bsc << endl;
        }

        if (decoder->m_nodes[i].word_id == -1) continue;
        set<int> la_states;
        find_preceeding_la_states(i, la_states, reverse_arcs);

        int word_id = decoder->m_nodes[i].word_id;

        vector<int> &pred_words = reverse_bigrams[word_id];

        for (auto pwit = pred_words.begin(); pwit != pred_words.end(); ++pwit) {
            float dummy_prob = 0.0;
            int nd = m_la_lm.score(m_la_lm.root_node, m_subword_id_to_la_ngram_symbol[*pwit], dummy_prob);
            float la_prob = 0.0;
            m_la_lm.score(nd, m_subword_id_to_la_ngram_symbol[word_id], la_prob);

            for (auto lasit = la_states.begin(); lasit != la_states.end(); ++lasit) {

                float unigram_prob = 0.0;
                m_la_lm.score(nd, m_subword_id_to_la_ngram_symbol[m_lookahead_states[*lasit].m_best_unigram_word_id], unigram_prob);
                // if (unigram_prob >= la_prob) continue;
                // OPTION 2: takes more memory, faster
                if (unigram_prob > la_prob) continue;

                map<int, float> &la_scores = m_lookahead_states[*lasit].m_scores;
                if (la_scores.find(*pwit) == la_scores.end()) {
                    la_scores[*pwit] = la_prob;
                    bigram_score_count++;
                }
                else
                    la_scores[*pwit] = max(la_prob, la_scores[*pwit]);
            }
        }
    }

    return bigram_score_count;
}


void
LargeBigramLookahead::write(string ofname)
{
    ofstream olafile(ofname);
    if (!olafile) throw string("Problem opening file: " + ofname);

    olafile << m_lookahead_states.size() << endl;
    for (int i=0; i<(int)m_lookahead_states.size(); i++) {
        LookaheadState &la_state = m_lookahead_states[i];
        olafile << i
                << " " << la_state.m_best_unigram_word_id
                << " " << la_state.m_best_unigram_score
                << " " << la_state.m_scores.size();
        for (auto bsit = la_state.m_scores.begin(); bsit != la_state.m_scores.end(); ++bsit)
            olafile << " " << bsit->first << " " << bsit->second;
        olafile << endl;
    }
}


void
LargeBigramLookahead::read(string ifname)
{
    ifstream ilafile(ifname);
    if (!ilafile) throw string("Problem opening file: " + ifname);

    string read_error("Problem reading ARPA file");
    string line;

    if (!getline(ilafile, line)) throw read_error;
    int la_state_count;

    stringstream ssline(line);
    ssline >> la_state_count;
    m_lookahead_states.resize(la_state_count);

    for (int i=0; i<la_state_count; i++) {

        if (!getline(ilafile, line)) throw read_error;
        stringstream ssline(line);

        LookaheadState &la_state = m_lookahead_states[i];

        int idx; ssline >> idx;
        if (i != idx) throw string("Index problem");

        ssline >> la_state.m_best_unigram_word_id;
        ssline >> la_state.m_best_unigram_score;

        int bigram_score_count; ssline >>bigram_score_count;
        for (int bsi=0; bsi<bigram_score_count; ++bsi) {
            int id; float score;
            ssline >>id >>score;
            la_state.m_scores[id] = score;
        }
    }
}

