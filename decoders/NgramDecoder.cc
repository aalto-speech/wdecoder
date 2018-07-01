#include <array>
#include <algorithm>
#include <sstream>
#include <ctime>

#include "NowayHmmReader.hh"
#include "NgramDecoder.hh"

using namespace std;


NgramDecoder::NgramDecoder()
{
    m_ngram_state_sentence_begin = -1;
}


NgramDecoder::~NgramDecoder()
{
}


void
NgramDecoder::read_lm(string lmfname)
{
    m_lm.read_arpa(lmfname);
    set_text_unit_id_ngram_symbol_mapping();

    if (m_use_word_boundary_symbol) {
        m_word_boundary_symbol_idx = m_text_unit_map[m_word_boundary_symbol];
        m_ngram_state_sentence_begin =
            m_lm.advance(m_lm.sentence_start_node,
                         m_text_unit_id_to_ngram_symbol[m_word_boundary_symbol_idx]);
    } else
        m_ngram_state_sentence_begin = m_lm.sentence_start_node;
}


void
NgramDecoder::set_text_unit_id_ngram_symbol_mapping()
{
    m_text_unit_id_to_ngram_symbol.resize(m_text_units.size(), -1);
    for (unsigned int i=0; i<m_text_units.size(); i++)
        m_text_unit_id_to_ngram_symbol[i] = m_lm.vocabulary_lookup[m_text_units[i]];
}


NgramRecognition::NgramRecognition(NgramDecoder &decoder)
    : Recognition::Recognition(decoder),
      m_ngram_state_sentence_begin(decoder.m_ngram_state_sentence_begin)
{
    ngd = static_cast<NgramDecoder*>(d);
    m_histogram_bin_limit = 0;
    m_word_history_leafs.clear();
    m_raw_tokens.clear();
    m_raw_tokens.reserve(1000000);
    m_recombined_tokens.clear();
    m_recombined_tokens.resize(d->m_nodes.size());
    m_best_node_scores.resize(d->m_nodes.size(), -1e20);
    m_active_nodes.clear();
    m_active_histories.clear();
    NgramToken tok;
    tok.d = &decoder;
    tok.lm_node = m_ngram_state_sentence_begin;
    tok.last_word_id = m_sentence_begin_symbol_idx;
    tok.history = new WordHistory();
    tok.history->word_id = m_sentence_begin_symbol_idx;
    m_history_root = tok.history;
    tok.node_idx = d->m_decode_start_node;
    m_active_nodes.insert(d->m_decode_start_node);
    m_word_history_leafs.insert(tok.history);
    if (m_use_word_boundary_symbol) {
        advance_in_word_history(&tok, m_word_boundary_symbol_idx);
        tok.last_word_id = m_word_boundary_symbol_idx;
    }
    m_active_histories.insert(tok.history);
    m_recombined_tokens[d->m_decode_start_node][tok.lm_node] = tok;
    m_total_token_count = 0;
}


void
NgramRecognition::reset_frame_variables()
{
    m_token_count = 0;
    m_best_log_prob = -1e20;
    m_best_word_end_prob = -1e20;
    m_global_beam_pruned_count = 0;
    m_max_state_duration_pruned_count = 0;
    m_word_end_beam_pruned_count = 0;
    m_node_beam_pruned_count = 0;
    m_histogram_pruned_count = 0;
    m_dropped_count = 0;
    m_token_count_after_pruning = 0;
    m_active_histories.clear();
    fill(m_best_node_scores.begin(), m_best_node_scores.end(), -1e20);
}


void
NgramRecognition::propagate_tokens()
{
    int node_count = 0;
    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit) {
        Decoder::Node &node = d->m_nodes[*nit];
        for (auto tit = m_recombined_tokens[*nit].begin(); tit != m_recombined_tokens[*nit].end(); ++tit) {

            if (tit->second.histogram_bin < m_histogram_bin_limit) {
                m_histogram_pruned_count++;
                continue;
            }

            m_token_count++;
            tit->second.word_end = false;

            for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
                move_token_to_node(tit->second, ait->target_node, ait->log_prob, ait->update_lookahead);
        }
        node_count++;
    }

    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit)
        m_recombined_tokens[*nit].clear();

    if (m_stats)
        cerr << "token count before propagation: " << m_token_count << endl;
}


void
NgramRecognition::prune_tokens(
    bool collect_active_histories,
    bool write_nbest)
{
    vector<NgramToken> pruned_tokens;
    pruned_tokens.reserve(50000);

    // Global beam pruning
    // Node beam pruning
    // Word end beam pruning
    float current_glob_beam = m_best_log_prob - m_global_beam;
    float current_word_end_beam = m_best_word_end_prob - m_word_end_beam;
    for (unsigned int i=0; i<m_raw_tokens.size(); i++) {
        NgramToken &tok = m_raw_tokens[i];

        if (tok.total_log_prob < current_glob_beam) {
            m_global_beam_pruned_count++;
            continue;
        }

        if (tok.total_log_prob < (m_best_node_scores[tok.node_idx] - m_node_beam)) {
            m_node_beam_pruned_count++;
            continue;
        }

        if (tok.word_end) {
            float prob_wo_la = tok.total_log_prob - d->m_lm_scale * tok.lookahead_log_prob;
            if (prob_wo_la < current_word_end_beam) {
                m_word_end_beam_pruned_count++;
                continue;
            }
        }

        tok.histogram_bin = (int) round((float)(HISTOGRAM_BIN_COUNT-1) * (tok.total_log_prob-current_glob_beam)/m_global_beam);
        pruned_tokens.push_back(tok);
    }
    m_raw_tokens.clear();

    // Recombine node/ngram state hypotheses
    m_active_nodes.clear();
    vector<int> histogram(HISTOGRAM_BIN_COUNT, 0);
    for (auto tit = pruned_tokens.begin(); tit != pruned_tokens.end(); tit++) {
        map<int, NgramToken> &node_tokens = m_recombined_tokens[tit->node_idx];
        auto bntit = node_tokens.find(tit->lm_node);
        if (bntit != node_tokens.end()) {
            if (tit->total_log_prob > bntit->second.total_log_prob) {
                histogram[bntit->second.histogram_bin]--;
                histogram[tit->histogram_bin]++;

                if (write_nbest && bntit->second.history != tit->history
                        && bntit->second.history != tit->history->previous
                        && bntit->second.history->previous != tit->history) {
                    array<float,3> weights = {
                        bntit->second.total_log_prob - tit->total_log_prob,
                        bntit->second.am_log_prob - tit->am_log_prob,
                        bntit->second.lm_log_prob - tit->lm_log_prob
                    };
                    WordHistory *previousBestHistory = bntit->second.history;

                    auto blit = tit->history->recombination_links.find(previousBestHistory);
                    if (blit == tit->history->recombination_links.end()) {
                        tit->history->recombination_links[previousBestHistory] = weights;
                        m_num_recombination_links++;
                    } else if (weights[0] > blit->second.at(0)) {
                        tit->history->recombination_links[previousBestHistory] = weights;
                        m_num_recombination_link_updated++;
                    } else
                        m_num_recombination_link_not_updated++;
                }

                bntit->second = *tit;
            }
            m_dropped_count++;
        }
        else {
            node_tokens[tit->lm_node] = *tit;
            m_active_nodes.insert(tit->node_idx);
            histogram[tit->histogram_bin]++;
            m_token_count_after_pruning++;
        }
    }

    if (collect_active_histories)
        for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit)
            for (auto tit = m_recombined_tokens[*nit].begin(); tit != m_recombined_tokens[*nit].end(); ++tit)
                m_active_histories.insert(tit->second.history);

    m_histogram_bin_limit = 0;
    int histogram_tok_count = 0;
    for (int i=HISTOGRAM_BIN_COUNT-1; i>= 0; i--) {
        histogram_tok_count += histogram[i];
        if (histogram_tok_count > d->m_token_limit) {
            m_histogram_bin_limit = i;
            break;
        }
    }

    if (m_stats) cerr << "token count after pruning: " << m_token_count_after_pruning << endl;
    if (m_stats) cerr << "histogram index: " << m_histogram_bin_limit << endl;
}


void
NgramRecognition::move_token_to_node(
    NgramToken token,
    int node_idx,
    float transition_score,
    bool update_lookahead)
{
    token.am_log_prob += m_transition_scale * transition_score;

    Decoder::Node &node = d->m_nodes[node_idx];

    if (token.node_idx == node_idx) {
        token.dur++;
        if (token.dur > m_max_state_duration && node.hmm_state > m_last_sil_idx) {
            m_max_state_duration_pruned_count++;
            return;
        }
    }
    else {
        // Apply duration model for previous state if moved out from a hmm state
        if (m_duration_model_in_use && d->m_nodes[token.node_idx].hmm_state != -1)
            token.apply_duration_model();
        token.node_idx = node_idx;
        token.dur = 1;
    }

    // HMM node
    if (node.hmm_state != -1) {

        if (update_lookahead)
            token.update_lookahead_prob(d->m_la->get_lookahead_score(node_idx, token.last_word_id));

        token.am_log_prob += m_acoustics->log_prob(node.hmm_state);

        token.update_total_log_prob();
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }

        m_best_log_prob = max(m_best_log_prob, token.total_log_prob);
        if (token.word_end) {
            float lp_wo_la = token.total_log_prob - d->m_lm_scale * token.lookahead_log_prob;
            m_best_word_end_prob = max(m_best_word_end_prob, lp_wo_la);
        }
        m_best_node_scores[node_idx] = max(m_best_node_scores[node_idx], token.total_log_prob);
        m_raw_tokens.push_back(token);
        return;
    }

    // LM node
    // Update LM score
    // Update history
    if (node.word_id != -1) {
        token.lm_node = ngd->m_lm.score(
                            token.lm_node,
                            ngd->m_text_unit_id_to_ngram_symbol[node.word_id], token.lm_log_prob);
        token.last_word_id = node.word_id;

        if (update_lookahead) {
            if ((node.word_id == m_sentence_end_symbol_idx) && m_use_word_boundary_symbol) {
                token.update_lookahead_prob(d->m_la->get_lookahead_score(node_idx, m_word_boundary_symbol_idx));
            }
            else if (node.word_id == m_sentence_end_symbol_idx) {
                token.update_lookahead_prob(d->m_la->get_lookahead_score(node_idx, m_sentence_begin_symbol_idx));
            }
            else {
                token.update_lookahead_prob(d->m_la->get_lookahead_score(node_idx, token.last_word_id));
            }
        }
        token.update_total_log_prob();
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }

        advance_in_word_history(&token, node.word_id);
        token.word_end = true;

        if (node.word_id == m_sentence_end_symbol_idx) {
            token.lm_node = m_ngram_state_sentence_begin;
            if (m_use_word_boundary_symbol) {
                advance_in_word_history(&token, m_word_boundary_symbol_idx);
                token.last_word_id = m_word_boundary_symbol_idx;
            }
            else {
                token.last_word_id = m_sentence_begin_symbol_idx;
            }
        }
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
        move_token_to_node(token, ait->target_node, ait->log_prob, ait->update_lookahead);
}


void
NgramRecognition::get_tokens(vector<Token*> &tokens)
{
    tokens.clear();
    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit) {
        map<int, NgramToken> & node_tokens = m_recombined_tokens[*nit];
        for (auto tit = node_tokens.begin(); tit != node_tokens.end(); ++tit)
            tokens.push_back(&(tit->second));
    }
}


void
NgramRecognition::add_sentence_ends(vector<Token*> &tokens)
{
    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        NgramToken *token = static_cast<NgramToken*>(*tit);
        if (token->lm_node == ngd->m_ngram_state_sentence_begin) continue;
        if (m_use_word_boundary_symbol && token->last_word_id != d->m_word_boundary_symbol_idx) {
            token->lm_node = ngd->m_lm.score(
                                 token->lm_node,
                                 ngd->m_text_unit_id_to_ngram_symbol[d->m_word_boundary_symbol_idx], token->lm_log_prob);
            token->update_total_log_prob();
            advance_in_word_history(token, d->m_text_unit_map[d->m_word_boundary_symbol]);
        }
        token->lm_node = ngd->m_lm.score(
                             token->lm_node,
                             ngd->m_text_unit_id_to_ngram_symbol[d->m_sentence_end_symbol_idx], token->lm_log_prob);
        token->update_total_log_prob();
        advance_in_word_history(token, m_sentence_end_symbol_idx);
    }
}


string
NgramRecognition::get_result(WordHistory *history)
{
    string result;
    vector<int> text_units;
    while (true) {
        text_units.push_back(history->word_id);
        if (history->previous == nullptr) break;
        history = history->previous;
    }

    if (m_use_word_boundary_symbol)
        for (auto swit = text_units.rbegin(); swit != text_units.rend(); ++swit)
            result += " " + m_text_units->at(*swit);
    else
        for (auto swit = text_units.rbegin(); swit != text_units.rend(); ++swit)
            if (*swit >= 0) result += " " + m_text_units->at(*swit);

    return result;
}
