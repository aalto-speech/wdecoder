#include <algorithm>
#include <sstream>
#include <ctime>

#include "NowayHmmReader.hh"
#include "Decoder.hh"

using namespace std;


Decoder::Decoder()
{
    m_stats = 0;
    m_duration_model_in_use = false;
    m_use_word_boundary_symbol = false;
    m_force_sentence_end = true;
    m_lm_scale = 0.0;
    m_duration_scale = 0.0;
    m_transition_scale = 0.0;
    m_word_boundary_symbol_idx = -1;
    m_sentence_begin_symbol_idx = -1;
    m_sentence_end_symbol_idx = -1;
    m_la = nullptr;
    m_global_beam = 0.0;
    m_word_end_beam = 0.0;
    m_node_beam = 0.0;
    m_token_limit = 500000;
    m_history_clean_frame_interval = 10;
    m_max_state_duration = 80;
    m_decode_start_node = -1;
    m_last_sil_idx = -1;
}


Decoder::~Decoder()
{
}


void
Decoder::read_phone_model(string phnfname)
{
    ifstream phnf(phnfname);
    if (!phnf) throw string("Problem opening phone model.");

    int modelcount;
    NowayHmmReader::read(phnf, m_hmms, m_hmm_map, m_hmm_states, modelcount);

    vector<string> sil_labels = { "_", "__", "_f", "_s" };
    for (auto lit=sil_labels.begin(); lit !=sil_labels.end(); ++lit) {
        if (m_hmm_map.find(*lit) == m_hmm_map.end()) continue;
        int hmm_idx = m_hmm_map[*lit];
        for (auto sit=m_hmms[hmm_idx].states.begin(); sit != m_hmms[hmm_idx].states.end(); ++sit)
            m_last_sil_idx = max(m_last_sil_idx, sit->model);
    }
}


void
Decoder::read_duration_model(string durfname)
{
    ifstream durf(durfname);
    if (!durf) throw string("Problem opening duration model.");
    NowayHmmReader::read_durations(durf, m_hmms, m_hmm_states);
    m_duration_model_in_use = true;
}


void
Decoder::read_noway_lexicon(string lexfname)
{
    ifstream lexf(lexfname);
    if (!lexf) throw string("Problem opening noway lexicon file.");

    string line;
    int linei = 1;
    while (getline(lexf, line)) {
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
            double prob = atof(unit.substr(leftp+1, rightp-leftp-1).c_str());
            unit = unit.substr(0, leftp);
        }

        bool problem_phone = false;
        for (auto pit = phones.begin(); pit != phones.end(); ++pit) {
            if (m_hmm_map.find(*pit) == m_hmm_map.end()) {
                cerr << "Unknown phone " + *pit << endl;
                problem_phone = true;
            }
        }
        if (problem_phone) continue;

        if (m_text_unit_map.find(unit) == m_text_unit_map.end()) {
            m_text_units.push_back(unit);
            m_text_unit_map[unit] = m_text_units.size()-1;
            if (unit == "<s>") m_sentence_begin_symbol_idx = m_text_units.size()-1;
            if (unit == "</s>") m_sentence_end_symbol_idx = m_text_units.size()-1;
        }
        m_lexicon[unit] = phones;

        linei++;
    }
}


void
Decoder::read_dgraph(string fname)
{
    SimpleFileInput ginf(fname);

    int node_idx, node_count, arc_count;
    string line;

    ginf.getline(line);
    stringstream ncl(line);
    ncl >> node_count;
    m_nodes.clear();
    m_nodes.resize(node_count);

    string ltype;
    for (unsigned int i=0; i<m_nodes.size(); i++) {
        ginf.getline(line);
        stringstream nss(line);
        nss >> ltype;
        if (ltype != "n") throw string("Problem reading graph.");
        Node &node = m_nodes[i];
        nss >> node_idx >> node.hmm_state >> node.word_id >> arc_count >> node.flags;
        if (node.flags & NODE_DECODE_START) m_decode_start_node = node_idx;
        node.arcs.resize(arc_count);
    }

    vector<int> node_arc_counts;
    node_arc_counts.resize(node_count, 0);
    while (ginf.getline(line)) {
        stringstream ass(line);
        int src_node, tgt_node;
        ass >> ltype >> src_node >> tgt_node;
        if (ltype != "a") throw string("Problem reading graph.");
        m_nodes[src_node].arcs[node_arc_counts[src_node]].target_node = tgt_node;
        node_arc_counts[src_node]++;
    }

    set_hmm_transition_probs();
}


void
Decoder::set_hmm_transition_probs()
{
    for (unsigned int i=0; i<m_nodes.size(); i++) {

        Node &node = m_nodes[i];
        if (node.hmm_state == -1) continue;

        HmmState &state = m_hmm_states[node.hmm_state];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            if (ait->target_node == (int)i) ait->log_prob = state.transitions[0].log_prob;
            else ait->log_prob = state.transitions[1].log_prob;
        }
    }
}


void
Decoder::get_reverse_arcs(vector<vector<Decoder::Arc> > &reverse_arcs)
{
    reverse_arcs.clear();
    reverse_arcs.resize(m_nodes.size());

    for (int ni = 0; ni < (int)(m_nodes.size()); ni++)
        for (auto ait = m_nodes[ni].arcs.begin(); ait != m_nodes[ni].arcs.end(); ++ait) {
            if (ni == ait->target_node) continue;
            reverse_arcs[ait->target_node].resize(reverse_arcs[ait->target_node].size()+1);
            reverse_arcs[ait->target_node].back().target_node = ni;
            reverse_arcs[ait->target_node].back().update_lookahead = ait->update_lookahead;
        }
}


void
Decoder::mark_initial_nodes(int max_depth,
                            int curr_depth,
                            int node_idx)
{
    Decoder::Node &node = m_nodes[node_idx];

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
Decoder::mark_tail_nodes(int max_depth,
                         vector<vector<Decoder::Arc> > &reverse_arcs,
                         int curr_depth,
                         int node_idx)
{
    Decoder::Node &node = m_nodes[node_idx];

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
Decoder::print_dot_digraph(vector<Node> &nodes,
                           ostream &fstr)
{
    fstr << "digraph {" << endl << endl;
    fstr << "\tnode [shape=ellipse,fontsize=30,fixedsize=false,width=0.95];" << endl;
    fstr << "\tedge [fontsize=12];" << endl;
    fstr << "\trankdir=LR;" << endl << endl;

    for (unsigned int nidx = 0; nidx < m_nodes.size(); ++nidx) {
        Node &nd = m_nodes[nidx];
        fstr << "\t" << nidx;
        if (nidx == START_NODE) fstr << " [label=\"start\"]" << endl;
        else if (nidx == END_NODE) fstr << " [label=\"end\"]" << endl;
        else if (nd.hmm_state != -1 && nd.word_id >= 0)
            fstr << " [label=\"" << nidx << ":" << nd.hmm_state << ", " << m_text_units[nd.word_id] << "\"]" << endl;
        else if (nd.hmm_state != -1 && nd.word_id == -1)
            fstr << " [label=\"" << nidx << ":"<< nd.hmm_state << "\"]" << endl;
        else if (nd.hmm_state == -1 && nd.word_id >= 0)
            fstr << " [label=\"" << nidx << ":"<< m_text_units[nd.word_id] << "\"]" << endl;
        else if (nd.hmm_state == -1 && nd.word_id == -2)
            fstr << " [label=\"" << nidx << ":dummy/wb\"]" << endl;
        else
            fstr << " [label=\"" << nidx << ":dummy\"]" << endl;
    }

    fstr << endl;
    for (unsigned int nidx = 0; nidx < m_nodes.size(); ++nidx) {
        Node &node = m_nodes[nidx];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            fstr << "\t" << nidx << " -> " << ait->target_node
                 << "[label=\"" << ait->log_prob << "\"];" << endl;
    }
    fstr << "}" << endl;
}


void
Recognition::Token::update_total_log_prob()
{
    total_log_prob = am_log_prob + (d->m_lm_scale * lm_log_prob);
}


void
Recognition::Token::apply_duration_model()
{
    am_log_prob += d->m_duration_scale
        * d->m_hmm_states[d->m_nodes[node_idx].hmm_state].duration.get_log_prob(dur);
}


void
Recognition::Token::update_lookahead_prob(float new_lookahead_prob)
{
    lm_log_prob -= lookahead_log_prob;
    lm_log_prob += new_lookahead_prob;
    lookahead_log_prob = new_lookahead_prob;
}


Recognition::Recognition(Decoder &decoder) :
    m_stats(decoder.m_stats),
    m_transition_scale(decoder.m_transition_scale),
    m_global_beam(decoder.m_global_beam),
    m_node_beam(decoder.m_node_beam),
    m_word_end_beam(decoder.m_word_end_beam),
    m_duration_model_in_use(decoder.m_duration_model_in_use),
    m_max_state_duration(decoder.m_max_state_duration),
    m_last_sil_idx(decoder.m_last_sil_idx),
    m_use_word_boundary_symbol(decoder.m_use_word_boundary_symbol),
    m_word_boundary_symbol_idx(decoder.m_word_boundary_symbol_idx),
    m_sentence_begin_symbol_idx(decoder.m_sentence_begin_symbol_idx),
    m_sentence_end_symbol_idx(decoder.m_sentence_end_symbol_idx)
{
    m_acoustics = nullptr;
    m_total_token_count = 0;
    m_token_count = 0;
    m_token_count_after_pruning = 0;
    m_dropped_count = 0;
    m_global_beam_pruned_count = 0;
    m_word_end_beam_pruned_count = 0;
    m_node_beam_pruned_count = 0;
    m_max_state_duration_pruned_count = 0;
    m_histogram_pruned_count = 0;
    m_best_log_prob = -1e20;
    m_best_word_end_prob = -1e20;
    m_histogram_bin_limit = 0;
    m_history_root = nullptr;
    m_frame_idx = -1;
    m_text_units = &decoder.m_text_units;
    d = &decoder;
}


void
Recognition::prune_word_history()
{
    for (auto whlnit = m_word_history_leafs.begin(); whlnit != m_word_history_leafs.end(); ) {
        WordHistory *wh = *whlnit;
        if (m_active_histories.find(wh) == m_active_histories.end()) {
            m_word_history_leafs.erase(whlnit++);
            while (true) {
                WordHistory *tmp = wh;
                wh = wh->previous;
                wh->next.erase(tmp->word_id);
                delete tmp;
                if (wh == nullptr || wh->next.size() > 0) break;
                if (m_active_histories.find(wh) != m_active_histories.end()) {
                    m_word_history_leafs.insert(wh);
                    break;
                }
            }
        }
        else ++whlnit;
    }
}


void
Recognition::clear_word_history()
{
    for (auto whlnit = m_word_history_leafs.begin(); whlnit != m_word_history_leafs.end(); ++whlnit) {
        WordHistory *wh = *whlnit;
        while (wh != nullptr) {
            if (wh->next.size() > 0) break;
            WordHistory *tmp = wh;
            wh = wh->previous;
            if (wh != nullptr) wh->next.erase(tmp->word_id);
            delete tmp;
        }
    }
    m_word_history_leafs.clear();
    m_active_histories.clear();
}


void
Recognition::print_certain_word_history(ostream &outf)
{
    WordHistory *hist = m_history_root;
    while (true) {
        if (hist->word_id >= 0)
            outf << m_text_units->at(hist->word_id) << " ";
        if (hist->next.size() > 1 || hist->next.size() == 0) break;
        else hist = hist->next.begin()->second;
    }
    outf << endl;
}


void
Recognition::advance_in_word_history(Token *token, int word_id)
{
    auto next_history = token->history->next.find(word_id);
    if (next_history != token->history->next.end())
        token->history = next_history->second;
    else {
        token->history = new WordHistory(word_id, token->history);
        token->history->previous->next[word_id] = token->history;
        m_word_history_leafs.erase(token->history->previous);
        m_word_history_leafs.insert(token->history);
    }
}


Recognition::Token*
Recognition::get_best_token(vector<Token*> &tokens)
{
    Token *best_token = nullptr;
    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        if (best_token == nullptr)
            best_token = *tit;
        else if ((*tit)->total_log_prob > best_token->total_log_prob)
            best_token = *tit;
    }
    return best_token;
}


Recognition::Token*
Recognition::get_best_end_token(vector<Token*> &tokens)
{
    Token *best_token = nullptr;
    for (auto tit = tokens.begin(); tit != tokens.end(); ++tit) {
        //if (tit->lm_node != m_ngram_state_sentence_begin) continue;
        Decoder::Node &node = d->m_nodes[(*tit)->node_idx];
        if (node.flags & NODE_SILENCE) {
            if (best_token == nullptr)
                best_token = *tit;
            else if ((*tit)->total_log_prob > best_token->total_log_prob)
                best_token = *tit;
        }
    }
    return best_token;
}


RecognitionResult::RecognitionResult()
{
    total_frames = 0;
    total_time = 0.0;
    total_lp = 0.0;
    total_am_lp = 0.0;
    total_lm_lp = 0.0;
    total_token_count = 0.0;
}


void
RecognitionResult::print_file_stats(ostream &statsf)
{
    statsf << "\trecognized " << total_frames << " frames in "
            << total_time << " seconds." << endl;
    statsf << "\tRTF: " << total_time / ((double)total_frames/125.0) << endl;
    statsf << "\tLog prob: " << total_lp << "\tAM: " << total_am_lp << "\tLM: " << total_lm_lp << endl;
    statsf << "\tMean token count: " << total_token_count / (double)total_frames << endl;

}


void
RecognitionResult::print_final_stats(ostream &statsf)
{
    statsf << "total recognition time: " << total_time << endl;
    statsf << "total frame count: " << total_frames << endl;
    statsf << "total RTF: " << total_time/ ((double)total_frames/125.0) << endl;
    statsf << "total log likelihood: " << total_lp << endl;
    statsf << "total LM likelihood: " << total_lm_lp << endl;
    statsf << "total AM likelihood: " << total_am_lp << endl;
    statsf << "total mean token count: " << total_token_count / (double)total_frames << endl;
}


void
RecognitionResult::accumulate(RecognitionResult &acc)
{
    total_frames += acc.total_frames;
    total_time += acc.total_time;
    total_lp += acc.total_lp;
    total_am_lp += acc.total_am_lp;
    total_lm_lp += acc.total_lm_lp;
    total_token_count += acc.total_token_count;
}

