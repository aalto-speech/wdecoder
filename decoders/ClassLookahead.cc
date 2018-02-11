#include <ctime>
#include <cassert>
#include <climits>
#include <algorithm>
#include <list>
#include <sstream>
#include <map>

#include "ClassLookahead.hh"

using namespace std;


DummyClassBigramLookahead::DummyClassBigramLookahead(Decoder &decoder,
                                                     string carpafname,
                                                     string cmempfname)
    : m_class_la(carpafname,
                 cmempfname,
                 decoder.m_text_units,
                 decoder.m_text_unit_map)
{
    this->decoder = &decoder;
}


float
DummyClassBigramLookahead::get_lookahead_score(
    int node_idx,
    int word_id)
{
    vector<int> successor_words;
    find_successor_words(node_idx, successor_words);

    float la_prob = MIN_LOG_PROB;
    int la_node = m_class_la.advance(m_class_la.m_class_ngram.root_node, word_id);
    for (auto swit = successor_words.begin(); swit != successor_words.end(); ++swit) {
        if ((*swit != decoder->m_sentence_end_symbol_idx)
            && (m_class_la.m_class_membership_lookup[*swit].first == -1)) continue;
        float curr_prob = 0.0;
        m_class_la.score(la_node, *swit, curr_prob);
        la_prob = max(la_prob, curr_prob);
    }

    static float ln_to_log10 = 1.0 / log(10.0);
    return ln_to_log10 * la_prob;
}


ClassBigramLookahead::ClassBigramLookahead(
    Decoder &decoder,
    string carpafname,
    string cmempfname,
    bool quantization)
    : m_class_la(carpafname,
                 cmempfname,
                 decoder.m_text_units,
                 decoder.m_text_unit_map),
      m_la_state_count(0),
      m_quantization(quantization)
{
    this->decoder = &decoder;

    time_t t1,t2,t3;
    t1 = time(0);
    cerr << "Setting look-ahead state indices" << endl;
    m_node_la_states.resize(decoder.m_nodes.size(), -1);
    m_la_state_count = set_la_state_indices_to_nodes();
    t2 = time(0);
    cerr << "elapsed time for setting indices: " << (t2-t1) << endl;
    cerr << "Number of look-ahead states: " << m_la_state_count << endl;
    cerr << "Setting look-ahead scores" << endl;
    set_la_scores();
    t3 = time(0);
    cerr << "elapsed time for setting scores: " << (t3-t2) << endl;
    set_arc_la_updates();
}


float
ClassBigramLookahead::get_lookahead_score(int node_idx, int word_id)
{
    static float ln_to_log10 = 1.0 / log(10.0);
    int word_class = m_class_la.m_class_membership_lookup[word_id].first;
    if (m_quantization) {
        unsigned short int qIdx = m_quant_bigram_lookup[m_node_la_states[node_idx]][word_class];
        return ln_to_log10 * m_quant_log_probs.getQuantizedLogProb(qIdx);
    } else {
        return ln_to_log10 * m_la_scores[m_node_la_states[node_idx]][word_class];
    }
}


int
ClassBigramLookahead::set_la_state_indices_to_nodes()
{
    vector<vector<Decoder::Arc> > reverse_arcs;
    get_reverse_arcs(reverse_arcs);

    vector<multimap<float, int> > words;
    words.resize(m_class_la.m_num_classes+2);
    int word_count = 0;
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {
        Decoder::Node &nd = decoder->m_nodes[i];
        // propagate also sentence end here
        if (nd.word_id != -1
            && m_class_la.m_class_memberships.find(decoder->m_text_units[nd.word_id])
                != m_class_la.m_class_memberships.end())
        {
            int classIdx = m_class_la.m_class_membership_lookup[nd.word_id].first;
            float cmemp = m_class_la.m_class_membership_lookup[nd.word_id].second;
            words[classIdx].insert(make_pair(cmemp, i));
            word_count++;
        }
    }

    long long int max_state_idx = 0;
    int wrdi = 0;
    for (int classIdx = 0; classIdx < (int)words.size(); classIdx++) {
        vector<bool> class_propagated(decoder->m_nodes.size(), false);
        multimap<float, int> &cwords = words[classIdx];
        for (auto wit = cwords.rbegin(); wit != cwords.rend(); ++wit) {
            int nodeIdx = wit->second;
            int wordId = decoder->m_nodes[nodeIdx].word_id;

            if (++wrdi % 100 == 0) {
                set<int> distLaStates;
                for (int i=0; i<(int)m_node_la_states.size(); i++)
                    distLaStates.insert(m_node_la_states[i]);
                cerr << wrdi << "/" << word_count << "\t" << distLaStates.size() << endl;
            }

            map<int, int> la_state_changes;
            vector<bool> processed_nodes(decoder->m_nodes.size(), false);
            list<int> nodes_to_process;
            for (auto rait=reverse_arcs[nodeIdx].begin(); rait!=reverse_arcs[nodeIdx].end(); ++rait) {
                if (rait->target_node == nodeIdx) continue;
                if (class_propagated[rait->target_node]) continue;
                nodes_to_process.push_back(rait->target_node);
            }

            while(nodes_to_process.size()) {
                int node_idx = nodes_to_process.front();
                nodes_to_process.pop_front();
                processed_nodes[node_idx] = true;
                if (class_propagated[node_idx]) continue;
                class_propagated[node_idx] = true;
                Decoder::Node &nd = decoder->m_nodes[node_idx];

                int curr_la_state = m_node_la_states[node_idx];
                auto la_state_change = la_state_changes.find(curr_la_state);
                if (la_state_change == la_state_changes.end()) {
                    la_state_changes[curr_la_state] = ++max_state_idx;
                    m_node_la_states[node_idx] = max_state_idx;
                } else
                    m_node_la_states[node_idx] = la_state_change->second;
                if (nd.word_id != -1) continue;
                if (wordId != decoder->m_sentence_end_symbol_idx
                    && node_idx == START_NODE) continue;

                for (auto rait=reverse_arcs[node_idx].begin(); rait!=reverse_arcs[node_idx].end(); ++rait) {
                    if (rait->target_node == node_idx) continue;
                    if (processed_nodes[rait->target_node]) continue;
                    if (class_propagated[rait->target_node]) continue;
                    nodes_to_process.push_back(rait->target_node);
                }
            }
        }
    }

    map<int, int> la_state_remapping;
    int remapped_la_idx = 0;
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++) {
        int curr_la_idx = m_node_la_states[i];
        if (la_state_remapping.find(curr_la_idx) == la_state_remapping.end())
            la_state_remapping[curr_la_idx] = remapped_la_idx++;
    }
    for (unsigned int i=0; i<decoder->m_nodes.size(); i++)
        m_node_la_states[i] = la_state_remapping[m_node_la_states[i]];

    return la_state_remapping.size();
}


float
ClassBigramLookahead::set_arc_la_updates()
{
    float update_count = 0.0;
    float no_update_count = 0.0;
    for (int i=0; i<(int)(decoder->m_nodes.size()); i++) {
        Decoder::Node &node = decoder->m_nodes[i];

        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            ait->update_lookahead = true;

        if (node.flags & NODE_DECODE_START) continue;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            int j = ait->target_node;

            if (m_node_la_states[i] == m_node_la_states[j]) {
                ait->update_lookahead = false;
                no_update_count += 1.0;
            }
            else
                update_count += 1.0;
        }
    }
    return update_count / (update_count + no_update_count);
}


void
ClassBigramLookahead::init_la_scores()
{
    if (m_quantization) {
        LNNgram &ngram = m_class_la.m_class_ngram;
        double min_backoff_prob = 0.0;
        for (int i=0; i<(int)ngram.nodes.size(); i++)
            min_backoff_prob = std::min(min_backoff_prob, ngram.nodes[i].backoff_prob);
        int first_root_arc = ngram.nodes[ngram.root_node].first_arc;
        int last_root_arc = ngram.nodes[ngram.root_node].last_arc;
        double min_root_word_prob = 0.0;
        for (int i=first_root_arc; i<=last_root_arc; i++) {
            int target_node = ngram.arc_target_nodes[i];
            double target_prob = ngram.nodes[target_node].prob;
            min_root_word_prob = std::min(min_root_word_prob, target_prob);
        }
        m_min_la_score = min_backoff_prob + min_root_word_prob;

        float min_cmemp = 0.0;
        for (auto wit = m_class_la.m_class_memberships.begin(); wit != m_class_la.m_class_memberships.end(); ++wit)
            min_cmemp = std::min(min_cmemp, wit->second.second);
        m_min_la_score += min_cmemp;

        m_quant_log_probs.setMinLogProb(m_min_la_score);

        m_quant_bigram_lookup.resize(m_la_state_count);
        for (auto blsit = m_quant_bigram_lookup.begin(); blsit != m_quant_bigram_lookup.end(); ++blsit)
            (*blsit).resize(m_class_la.m_num_classes+1, USHRT_MAX);
    } else {
        m_la_scores.resize(m_la_state_count);
        for (int i=0; i<(int)m_la_scores.size(); i++)
            m_la_scores[i].resize(m_class_la.m_num_classes+1, MIN_LOG_PROB);
    }
}


void
ClassBigramLookahead::set_la_score(
    int la_state,
    int class_idx,
    float la_prob)
{
    if (m_quantization)
        m_quant_bigram_lookup[la_state][class_idx] = m_quant_log_probs.getQuantIndex(la_prob);
    else {
        m_la_scores[la_state][class_idx] = max(m_la_scores[la_state][class_idx], la_prob);
    }
}


void
ClassBigramLookahead::set_la_scores()
{
    vector<vector<float> > cbgProbs;
    cbgProbs.resize(m_class_la.m_num_classes);
    for (int i=0; i<(int)cbgProbs.size(); i++) {
        cbgProbs[i].resize(m_class_la.m_num_classes+1, 0.0);
        int cng_node = m_class_la.m_class_ngram.advance(
                m_class_la.m_class_ngram.root_node, m_class_la.m_class_intmap[i]);
        for (int j=0; j<(int)cbgProbs[i].size()-1; j++)
            m_class_la.m_class_ngram.score(cng_node, m_class_la.m_class_intmap[j],
                    cbgProbs[i][j]);
        m_class_la.m_class_ngram.score(cng_node, m_class_la.m_class_ngram.sentence_end_symbol_idx,
                cbgProbs[i][cbgProbs[i].size()-1]);
    }

    map<int,int> la_state_nodes;
    for (int i=0; i<(int)m_node_la_states.size(); i++)
        la_state_nodes[m_node_la_states[i]] = i;

    init_la_scores();

    for (int i=0; i<m_la_state_count; i++) {
        int node_idx = la_state_nodes[i];
        vector<int> successor_words;
        find_successor_words(node_idx, successor_words);

        for (int c=0; c<m_class_la.m_num_classes; c++) {
            float best_la_prob = MIN_LOG_PROB;
            for (auto swit = successor_words.begin(); swit != successor_words.end(); ++swit)
            {
                if (*swit == decoder->m_sentence_end_symbol_idx) {
                    float curr_prob = cbgProbs[c].back();
                    best_la_prob = max(best_la_prob, curr_prob);
                } else {
                    pair<int, float> wordClassInfo = m_class_la.m_class_membership_lookup[*swit];
                    if (wordClassInfo.first == -1) continue;
                    float curr_prob = cbgProbs[c][wordClassInfo.first] + wordClassInfo.second;
                    best_la_prob = max(best_la_prob, curr_prob);
                }
            }
            set_la_score(i, c, best_la_prob);
        }

        // handle <s> as the context word
        float best_la_prob = MIN_LOG_PROB;
        for (auto swit = successor_words.begin(); swit != successor_words.end(); ++swit) {
            if (m_class_la.m_class_membership_lookup[*swit].first == -1) continue;
            float curr_prob = 0.0;
            m_class_la.score(m_class_la.m_class_ngram.sentence_start_node, *swit, curr_prob);
            best_la_prob = max(best_la_prob, curr_prob);
        }
        set_la_score(i, m_class_la.m_num_classes, best_la_prob);
    }
}
