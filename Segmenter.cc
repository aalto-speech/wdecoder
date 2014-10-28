#include <cmath>

#include "Segmenter.hh"

using namespace std;


Segmenter::Segmenter()
{
    m_state_history_root = nullptr;
    m_best_log_prob = -1e20;
    m_best_word_end_prob = -1e20;

}


void
Segmenter::initialize()
{
    m_raw_tokens.clear();
    m_recombined_tokens.clear();
    m_recombined_tokens.resize(m_nodes.size());
    m_active_nodes.clear();
    m_active_state_histories.clear();
    SToken tok;
    m_ngram_state_sentence_begin = tok.lm_node;
    tok.state_history = new StateHistory();
    tok.lm_node = m_lm.sentence_start_node;
    tok.node_idx = m_decode_start_node;
    m_active_nodes.insert(m_decode_start_node);
    m_active_state_histories.insert(tok.state_history);
    m_recombined_tokens[m_decode_start_node][tok.lm_node] = tok;
}


void
Segmenter::advance_in_state_history(SToken &token)
{
    int node_idx = token.node_idx;
    int hmm_state = m_nodes[node_idx].hmm_state;

    auto next_history = token.state_history->next.find(hmm_state);
    if (next_history != token.state_history->next.end())
        token.state_history = next_history->second;
    else {
        token.state_history = new StateHistory(hmm_state, token.state_history);
        token.state_history->previous->next[hmm_state] = token.state_history;
        m_state_history_leafs.erase(token.state_history->previous);
        m_state_history_leafs.insert(token.state_history);
    }

    if (token.am_log_prob > token.state_history->best_am_log_prob) {
        token.state_history->best_am_log_prob = token.am_log_prob;
        token.state_history->end_frame = m_frame_idx;
        token.state_history->start_frame = m_frame_idx-token.dur;
        if (m_state_history_labels.find(node_idx) != m_state_history_labels.end())
            token.state_history->label = m_state_history_labels[node_idx];
    }
}


void
Segmenter::prune_state_history()
{
    for (auto shlnit = m_state_history_leafs.begin(); shlnit != m_state_history_leafs.end(); ) {
        StateHistory *sh = *shlnit;
        if (m_active_state_histories.find(sh) == m_active_state_histories.end()) {
            m_state_history_leafs.erase(shlnit++);
            while (true) {
                StateHistory *tmp = sh;
                sh = sh->previous;
                sh->next.erase(tmp->hmm_state);
                delete tmp;
                if (sh == nullptr || sh->next.size() > 0) break;
                if (m_active_state_histories.find(sh) != m_active_state_histories.end()) {
                    m_state_history_leafs.insert(sh);
                    break;
                }
            }
        }
        else ++shlnit;
    }
}


void
Segmenter::clear_state_history()
{
    for (auto shlnit = m_state_history_leafs.begin(); shlnit != m_state_history_leafs.end(); ++shlnit) {
        StateHistory *sh = *shlnit;
        while (sh != nullptr) {
            if (sh->next.size() > 0) break;
            StateHistory *tmp = sh;
            sh = sh->previous;
            if (sh != nullptr) sh->next.erase(tmp->hmm_state);
            delete tmp;
        }
    }
    m_state_history_leafs.clear();
}


void
Segmenter::print_phn_segmentation(StateHistory *history,
                                  ostream &outf)
{
    vector<string> segmentation;
    while (true) {

        if (history->label.length() > 0) {
            int start_sample = round(float(history->start_frame) * 16000.0 / 125.0);
            int end_sample = round(float(history->end_frame) * 16000.0 / 125.0);
            string segline = to_string(start_sample)
                             + " "
                             + to_string(end_sample)
                             + " "
                             + history->label;
            segmentation.push_back(segline);
        }

        if (history->previous == nullptr) break;
        history = history->previous;
    }

    for (auto sit = segmentation.rbegin(); sit != segmentation.rend(); ++sit)
        outf << *sit << endl;
}


void
Segmenter::segment_lna_file(string lnafname,
                            map<int, string> &node_labels,
                            ostream &outf)
{
    m_state_history_labels = node_labels;
    m_lna_reader.open_file(lnafname, 1024);
    m_acoustics = &m_lna_reader;
    initialize();

    m_frame_idx = 0;
    while (m_lna_reader.go_to(m_frame_idx)) {
        propagate_tokens();
        recombine_tokens();
        m_frame_idx++;
    }

    vector<SToken> tokens;
    map<int, SToken> &node_tokens = m_recombined_tokens.back();
    for (auto tit = node_tokens.begin(); tit != node_tokens.end(); ++tit)
        tokens.push_back(tit->second);
    if (tokens.size() == 0) {
        cerr << "warning, no segmentation found" << endl;
        return;
    }

    SToken best_token;
    float best_lp = -1e20;
    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        SToken &tok = *tit;
        if (m_duration_model_in_use && tok.dur > 1) apply_duration_model(tok, tok.node_idx);
        advance_in_state_history(tok);
        tok.total_log_prob = get_token_log_prob(tok);
        if (tok.total_log_prob > best_lp) best_token = tok;
    }

    print_phn_segmentation(best_token.state_history, outf);

    clear_word_history();
    clear_state_history();
    m_lna_reader.close();
}


void
Segmenter::move_token_to_node(SToken token,
                              int node_idx,
                              float transition_score,
                              bool update_lookahead)
{
    token.am_log_prob += m_transition_scale * transition_score;

    Node &node = m_nodes[node_idx];

    if (token.node_idx == node_idx) {
        token.dur++;
    }
    else {
        // Apply duration model for previous state if moved out from a hmm state
        if (m_duration_model_in_use && m_nodes[token.node_idx].hmm_state != -1)
            apply_duration_model(token, token.node_idx);
        advance_in_state_history(token);
        token.node_idx = node_idx;
        token.dur = 1;
    }

    if (node.hmm_state != -1) {
        token.am_log_prob += m_acoustics->log_prob(node.hmm_state);
        token.total_log_prob = get_token_log_prob(token);
        m_raw_tokens.push_back(token);
        return;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
        move_token_to_node(token, ait->target_node, ait->log_prob, ait->update_lookahead);
}


void
Segmenter::propagate_tokens()
{
    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit) {
        Node &node = m_nodes[*nit];
        for (auto tit = m_recombined_tokens[*nit].begin(); tit != m_recombined_tokens[*nit].end(); ++tit) {
            for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
                move_token_to_node(tit->second, ait->target_node, ait->log_prob, ait->update_lookahead);
        }
    }

    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit)
        m_recombined_tokens[*nit].clear();
}


void
Segmenter::recombine_tokens()
{
    m_active_nodes.clear();
    for (auto tit = m_raw_tokens.begin(); tit != m_raw_tokens.end(); tit++) {
        map<int, SToken> &node_tokens = m_recombined_tokens[tit->node_idx];
        auto bntit = node_tokens.find(tit->lm_node);
        if (bntit != node_tokens.end()) {
            if (tit->total_log_prob > bntit->second.total_log_prob)
                bntit->second = *tit;
        }
        else {
            node_tokens[tit->lm_node] = *tit;
            m_active_nodes.insert(tit->node_idx);
        }
    }
}

