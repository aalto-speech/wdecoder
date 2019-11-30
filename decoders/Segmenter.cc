#include <cmath>
#include <algorithm>

#include "Segmenter.hh"

using namespace std;


Segmenter::Segmenter()
{
    m_best_log_prob = 0.0;
    m_transition_scale = 1.0;
    m_duration_scale = 3.0;
}


void
Segmenter::initialize()
{
    Token tok;
    tok.node_idx = m_decode_start_node;
    m_recombined_tokens.clear();
    m_recombined_tokens[m_decode_start_node] = tok;
}


void
Segmenter::advance_in_state_history(Token &token)
{
    StateHistory sh(token.node_idx);
    sh.end_frame = m_frame_idx;
    sh.start_frame = m_frame_idx-token.dur;
    token.state_history.push_back(sh);
}


void
Segmenter::print_phn_segmentation(Token &token,
                                  ostream &outf)
{

    for (auto sit = token.state_history.begin(); sit != token.state_history.end(); ++sit)
    {
        StateHistory &sh = *sit;
        string label = m_state_history_labels[sh.node_idx];

        if (label.length() > 0) {
            int start_sample = round(float(sh.start_frame) * 16000.0 / 125.0);
            int end_sample = round(float(sh.end_frame) * 16000.0 / 125.0);
            string segline = int2str(start_sample)
                             + " "
                             + int2str(end_sample)
                             + " "
                             + label;
            outf << segline << endl;
        }
    }
}


float
Segmenter::segment_lna_file(string lnafname,
                            map<int, string> &node_labels,
                            ostream *outf) {
    m_state_history_labels = node_labels;
    m_lna_reader.open_file(lnafname, 1024);
    m_acoustics = &m_lna_reader;
    initialize();

    m_frame_idx = 0;
    while (m_lna_reader.go_to(m_frame_idx)) {
        float curr_prob_limit = m_best_log_prob - m_global_beam;
        m_best_log_prob = TINY_FLOAT;
        propagate_tokens(curr_prob_limit);
        m_frame_idx++;
    }
    m_lna_reader.close();

    Token *best_token = nullptr;
    for (auto enit = m_decode_end_nodes.begin(); enit != m_decode_end_nodes.end(); ++enit) {
        Token *curr_tok = &(m_recombined_tokens[*enit]);
        if (best_token == nullptr || curr_tok->log_prob > best_token->log_prob)
            best_token = curr_tok;
    }

    if (best_token->node_idx == -1) {
        cerr << "warning, no segmentation found" << endl;
        return TINY_FLOAT;
    }

    if (outf != nullptr) {
        advance_in_state_history(*best_token);
        print_phn_segmentation(*best_token, *outf);
    }

    return best_token->log_prob;
}


void
Segmenter::apply_duration_model(Token &token, int node_idx)
{
    token.log_prob += m_duration_scale
                         * m_hmm_states[m_nodes[node_idx].hmm_state].duration.get_log_prob(token.dur);
}


void
Segmenter::move_token_to_node(Token token,
                              int node_idx,
                              float transition_score)
{
    token.log_prob += m_transition_scale * transition_score;

    Node &node = m_nodes[node_idx];

    if (token.node_idx == node_idx) {
        if (node.hmm_state != -1) token.dur++;
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
        token.log_prob += m_acoustics->log_prob(node.hmm_state);
        if (token.log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }
        m_best_log_prob = max(m_best_log_prob, token.log_prob);
        if (m_recombined_tokens.find(node_idx) == m_recombined_tokens.end() ||
            token.log_prob > m_recombined_tokens[node_idx].log_prob)
                m_recombined_tokens[node_idx] = token;
        return;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
        move_token_to_node(token, ait->target_node, ait->log_prob);
}


void
Segmenter::propagate_tokens(float curr_prob_limit)
{
    m_previous_recombined_tokens.swap(m_recombined_tokens);
    m_recombined_tokens.clear();
    for (auto rtit = m_previous_recombined_tokens.begin(); rtit != m_previous_recombined_tokens.end(); ++rtit)
    {
        if (rtit->second.log_prob < curr_prob_limit) continue;
        Node &node = m_nodes[rtit->second.node_idx];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            move_token_to_node(rtit->second, ait->target_node, ait->log_prob);
    }
}
