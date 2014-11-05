#include <cmath>

#include "Segmenter.hh"

using namespace std;


Segmenter::Segmenter()
{
    m_best_log_prob = -1e20;
    m_best_word_end_prob = -1e20;
    m_transition_scale = 1.0;
    m_duration_scale = 3.0;
}


void
Segmenter::initialize()
{
    m_raw_tokens.clear();
    m_recombined_tokens.clear();
    m_recombined_tokens.resize(m_nodes.size());
    m_active_nodes.clear();
    SToken tok;
    m_ngram_state_sentence_begin = tok.lm_node;
    tok.lm_node = m_lm.sentence_start_node;
    tok.node_idx = m_decode_start_node;
    m_active_nodes.insert(m_decode_start_node);
    m_recombined_tokens[m_decode_start_node] = tok;
}


void
Segmenter::advance_in_state_history(SToken &token)
{
    int node_idx = token.node_idx;
    int hmm_state = m_nodes[node_idx].hmm_state;

    StateHistory sh(hmm_state);
    sh.end_frame = m_frame_idx;
    sh.start_frame = m_frame_idx-token.dur;
    if (m_state_history_labels.find(node_idx) != m_state_history_labels.end())
        sh.label = m_state_history_labels[node_idx];

    token.state_history.push_back(sh);
}


void
Segmenter::print_phn_segmentation(SToken &token,
                                  ostream &outf)
{

    for (auto sit = token.state_history.begin(); sit != token.state_history.end(); ++sit)
    {
        StateHistory &sh = *sit;

        if (sh.label.length() > 0) {
            int start_sample = round(float(sh.start_frame) * 16000.0 / 125.0);
            int end_sample = round(float(sh.end_frame) * 16000.0 / 125.0);
            string segline = to_string(start_sample)
                             + " "
                             + to_string(end_sample)
                             + " "
                             + sh.label;
            outf << segline << endl;
        }
    }
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
        m_best_log_prob = -1e20;
        propagate_tokens();
        recombine_tokens();
        m_frame_idx++;
//        cerr << "frame: " << m_frame_idx << ", tokens " << m_active_nodes.size() << endl;
    }

    SToken &best_token = m_recombined_tokens.back();
    if (best_token.node_idx == -1) {
        cerr << "warning, no segmentation found" << endl;
        return;
    }

    advance_in_state_history(best_token);
    print_phn_segmentation(best_token, outf);

    clear_word_history();
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

    if (node.hmm_state != -1) {
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

        token.am_log_prob += m_acoustics->log_prob(node.hmm_state);
        token.total_log_prob = get_token_log_prob(token);
//        cerr << token.total_log_prob << endl;
        if (token.total_log_prob < (m_best_log_prob-m_global_beam)) {
            m_global_beam_pruned_count++;
            return;
        }
        m_best_log_prob = max(m_best_log_prob, token.total_log_prob);
        m_raw_tokens.push_back(token);
        return;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
        move_token_to_node(token, ait->target_node, ait->log_prob, false);
}


void
Segmenter::propagate_tokens()
{
    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit) {
        Node &node = m_nodes[*nit];
        SToken &tok = m_recombined_tokens[*nit];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            move_token_to_node(tok, ait->target_node, ait->log_prob, ait->update_lookahead);
    }

    m_active_nodes.clear();
}


void
Segmenter::recombine_tokens()
{
    m_recombined_tokens.clear();
    m_recombined_tokens.resize(m_nodes.size());

    for (auto tit = m_raw_tokens.begin(); tit != m_raw_tokens.end(); tit++) {
        if (tit->total_log_prob > m_recombined_tokens[tit->node_idx].total_log_prob) {
            m_recombined_tokens[tit->node_idx] = *tit;
            m_active_nodes.insert(tit->node_idx);
        }
    }

    m_raw_tokens.clear();
}

