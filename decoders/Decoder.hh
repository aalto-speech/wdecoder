#ifndef DECODER_HH
#define DECODER_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "Ngram.hh"
#include "LnaReaderCircular.hh"

#define HISTOGRAM_BIN_COUNT 100


class Decoder {

public:

    class Arc {
    public:
        Arc() : log_prob(0.0), target_node(-1), update_lookahead(false) { }
        float log_prob;
        int target_node;
        bool update_lookahead;
    };

    class Node {
    public:
        Node() : word_id(-1), hmm_state(-1), flags(0) { }
        int word_id; // -1 for nodes without word identity.
        int hmm_state; // -1 for nodes without acoustics.
        int flags;
        std::vector<Arc> arcs;
    };

    class WordHistory {
    public:
        WordHistory()
            : word_id(-1), previous(nullptr) { }
        WordHistory(int word_id, WordHistory *previous)
            : word_id(word_id), previous(previous) { }
        int word_id;
        WordHistory *previous;
        std::map<int, WordHistory*> next;
    };

    class Recognition {
    public:
        Recognition(Decoder &decoder);
        void active_nodes_sorted_by_best_lp(std::vector<int> &nodes);
        void prune_word_history();
        void clear_word_history();
        void print_certain_word_history(std::ostream &outf=std::cout);

        LnaReaderCircular m_lna_reader;
        Acoustics *m_acoustics;

        WordHistory* m_history_root;
        std::set<WordHistory*> m_word_history_leafs;
        std::set<WordHistory*> m_active_histories;

        std::set<int> m_active_nodes;
        std::vector<float> m_best_node_scores;

        // Passed from Decoder
        int m_stats;
        float m_lm_scale;
        float m_duration_scale;
        float m_transition_scale;
        float m_global_beam;
        float m_node_beam;
        float m_word_end_beam;
        bool m_duration_model_in_use;
        int m_max_state_duration;
        int m_last_sil_idx;
        bool m_use_word_boundary_symbol;
        int m_word_boundary_symbol_idx;
        int m_sentence_begin_symbol_idx;
        int m_sentence_end_symbol_idx;

        // Only in recognition
        int m_token_count;
        int m_token_count_after_pruning;
        float m_best_log_prob;
        float m_best_word_end_prob;
        int m_histogram_bin_limit;
        double m_total_token_count;
        int m_global_beam_pruned_count;
        int m_word_end_beam_pruned_count;
        int m_node_beam_pruned_count;
        int m_max_state_duration_pruned_count;
        int m_histogram_pruned_count;
        int m_dropped_count;
        int m_frame_idx;

        std::vector<std::string> *m_text_units;
    };

    class Lookahead {
    public:
        virtual float get_lookahead_score(int node_idx, int word_id) = 0;
        void set_text_unit_id_la_ngram_symbol_mapping();
        void get_reverse_arcs(std::vector<std::vector<Arc> > &reverse_arcs);
        void find_successor_words(int node_idx, std::vector<int> &word_ids);
        void find_successor_words(int node_idx, std::set<int> &word_ids, bool start_node=true);
        void find_predecessor_words(int node_idx,
                                    std::set<int> &word_ids,
                                    const std::vector<std::vector<Arc> > &reverse_arcs);
        bool detect_one_predecessor_node(int node_idx,
                                         int &predecessor_count,
                                         const std::vector<std::vector<Arc> > &reverse_arcs);
        void mark_initial_nodes(int max_depth, int curr_depth=0, int node=START_NODE);
        void mark_tail_nodes(int max_depth,
                             std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                             int curr_depth=0,
                             int node=END_NODE);
        virtual ~Lookahead() {};
        virtual void write(std::string ofname) { };
        virtual void read(std::string ifname) { };

        Decoder *decoder;
        Ngram m_la_lm;
        std::vector<int> m_text_unit_id_to_la_ngram_symbol;
    };

    Decoder();
    ~Decoder();

    void read_phone_model(std::string phnfname);
    void read_duration_model(std::string durfname);
    void read_noway_lexicon(std::string lexfname);
    void read_dgraph(std::string graphfname);
    void set_hmm_transition_probs();
    void print_dot_digraph(std::vector<Node> &nodes, std::ostream &fstr);

    // Words or subwords
    std::vector<std::string> m_text_units;
    // Mapping from text units to indices
    std::map<std::string, int> m_text_unit_map;
    // Vocabulary units as phone strings, FIXME: currently only one pronunciation
    std::map<std::string, std::vector<std::string> > m_lexicon;
    // Mapping from phones (triphones) to HMM indices.
    std::map<std::string,int> m_hmm_map;
    // The HMMs.
    std::vector<Hmm> m_hmms;
    // Hmm states
    std::vector<HmmState> m_hmm_states;

    // Lookahead language model
    Lookahead *m_la;

    std::vector<Node> m_nodes;

    int m_stats;

    float m_lm_scale;
    float m_duration_scale;
    float m_transition_scale;
    int m_token_limit;
    bool m_force_sentence_end;
    bool m_duration_model_in_use;
    int m_max_state_duration;

    bool m_use_word_boundary_symbol;
    std::string m_word_boundary_symbol;
    int m_word_boundary_symbol_idx;
    int m_sentence_begin_symbol_idx;
    int m_sentence_end_symbol_idx;

    float m_global_beam;
    float m_node_beam;
    float m_word_end_beam;

    int m_history_clean_frame_interval;
    int m_decode_start_node;
    int m_last_sil_idx;
};

#endif /* DECODER_HH */