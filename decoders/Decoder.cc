#include <algorithm>
#include <sstream>
#include <ctime>

#include "NowayHmmReader.hh"
#include "Decoder.hh"

using namespace std;


Decoder::Decoder()
{
    m_stats = 0;

    m_total_token_count = 0;

    m_duration_model_in_use = false;
    m_use_word_boundary_symbol = false;
    m_force_sentence_end = true;

    m_lm_scale = 0.0;
    m_duration_scale = 0.0;
    m_transition_scale = 0.0;
    m_token_count = 0;
    m_token_count_after_pruning = 0;
    m_word_boundary_symbol_idx = -1;
    m_sentence_begin_symbol_idx = -1;
    m_sentence_end_symbol_idx = -1;

    m_dropped_count = 0;
    m_global_beam_pruned_count = 0;
    m_word_end_beam_pruned_count = 0;
    m_node_beam_pruned_count = 0;
    m_max_state_duration_pruned_count = 0;
    m_histogram_pruned_count = 0;

    m_acoustics = nullptr;

    m_la = nullptr;

    m_best_log_prob = -1e20;
    m_best_word_end_prob = -1e20;
    m_histogram_bin_limit = 0;

    m_global_beam = 0.0;
    m_word_end_beam = 0.0;
    m_node_beam = 0.0;

    m_token_limit = 500000;

    m_history_root = nullptr;

    m_history_clean_frame_interval = 10;

    m_max_state_duration = 80;

    m_ngram_state_sentence_begin = -1;
    m_decode_start_node = -1;
    m_frame_idx = -1;

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
Decoder::active_nodes_sorted_by_best_lp(vector<int> &nodes)
{
    nodes.clear();
    vector<pair<int, float> > sorted_nodes;
    for (auto nit = m_active_nodes.begin(); nit != m_active_nodes.end(); ++nit)
        sorted_nodes.push_back(make_pair(*nit, m_best_node_scores[*nit]));
    sort(sorted_nodes.begin(), sorted_nodes.end(), descending_node_sort);
    for (auto snit = sorted_nodes.begin(); snit != sorted_nodes.end(); ++snit)
        nodes.push_back(snit->first);
}


void
Decoder::prune_word_history()
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
Decoder::clear_word_history()
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
Decoder::print_certain_word_history(ostream &outf)
{
    WordHistory *hist = m_history_root;
    while (true) {
        if (hist->word_id >= 0)
            outf << m_text_units[hist->word_id] << " ";
        if (hist->next.size() > 1 || hist->next.size() == 0) break;
        else hist = hist->next.begin()->second;
    }
    outf << endl;
}


