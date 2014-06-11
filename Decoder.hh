#ifndef DECODER_HH
#define DECODER_HH

#include <map>
#include <unordered_map>
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
    static const int WORD_BOUNDARY_IDENTIFIER;
    int SENTENCE_END_WORD_ID;

    class Arc {
    public:
        Arc() : log_prob(0.0), target_node(-1) { }
        float log_prob;
        int target_node;
    };

    class Node {
    public:
        Node()
            : word_id(-1), hmm_state(-1),
              flags(0), unigram_la_score(0.0),
              la_state_idx(-1) { }
        int word_id; // -1 for nodes without word identity.
        int hmm_state; // -1 for nodes without acoustics.
        int flags;
        std::vector<Arc> arcs;
        float unigram_la_score;
        int la_state_idx;
    };

    class Token;

    class WordHistory {
    public:
        WordHistory()
            : word_id(-1), previous(nullptr), best_am_log_prob(-1e20) { }
        WordHistory(int word_id, WordHistory *previous)
            : word_id(word_id), previous(previous), best_am_log_prob(-1e20) { }
        int word_id;
        WordHistory *previous;
        std::map<int, WordHistory*> next;
        float best_am_log_prob;
    };

    class Token {
    public:
        int node_idx;
        float am_log_prob;
        float lm_log_prob;
        float lookahead_log_prob;
        float total_log_prob;
        int lm_node;
        int last_word_id;
        WordHistory *history;
        unsigned short int dur;
        bool word_end;
        int histogram_bin;

        Token():
            node_idx(-1),
            am_log_prob(0.0f),
            lm_log_prob(0.0f),
            lookahead_log_prob(0.0f),
            total_log_prob(-1e20),
            lm_node(0),
            last_word_id(-1),
            history(nullptr),
            dur(0),
            word_end(false),
            histogram_bin(0)
        { }
    };

    Decoder();
    ~Decoder();

    void read_phone_model(std::string phnfname);
    void read_duration_model(std::string durfname);
    void read_noway_lexicon(std::string lexfname);
    void read_lm(std::string lmfname);
    void read_la_lm(std::string lmfname);
    void read_dgraph(std::string graphfname);

    void recognize_lna_file(std::string lnafname,
                            std::ostream &outf=std::cout,
                            int *frame_count=nullptr,
                            double *seconds=nullptr,
                            double *log_prob=nullptr,
                            double *am_prob=nullptr,
                            double *lm_prob=nullptr);

    void initialize();
    void reset_frame_variables();
    void propagate_tokens();
    void prune_tokens(bool collect_active_histories=false);
    void move_token_to_node(Token token, int node_idx, float transition_score);
    inline float get_token_log_prob(float am_score, float lm_score);
    inline void advance_in_history(Token& token, int word_id);
    inline void apply_duration_model(Token &token, int node_idx);
    inline void update_lookahead_prob(Token &token, float lookahead_prob);
    Token* get_best_token();
    Token get_best_token(std::vector<Token> &tokens);
    void add_sentence_ends(std::vector<Token> &tokens);
    void print_certain_word_history(std::ostream &outf=std::cout);
    void print_best_word_history(std::ostream &outf=std::cout);
    void print_word_history(WordHistory *history,
                            std::ostream &outf=std::cout,
                            bool print_lm_probs=false);
    void print_dot_digraph(std::vector<Node> &nodes, std::ostream &fstr);
    float score_state_path(std::string lnafname,
                           std::string sfname,
                           bool duration_model=true);
    void set_subword_id_ngram_symbol_mapping();
    void set_subword_id_la_ngram_symbol_mapping();
    void clear_word_history();
    void prune_word_history();
    void find_nodes_in_depth(std::set<int> &found_nodes,
                             int target_depth,
                             int curr_depth,
                             int curr_node);
    void set_hmm_transition_probs();
    void mark_initial_nodes(int max_depth, int curr_depth=0, int node=START_NODE);
    void active_nodes_sorted_by_best_lp(std::vector<int> &nodes);
    void reset_history_scores();
    void find_paths(std::vector<std::vector<int> > &paths,
                    std::vector<int> &words,
                    int curr_word_pos = 0,
                    std::vector<int> curr_path = std::vector<int>(),
                    int curr_node_idx = -1);
    void path_to_graph(std::vector<int> &path,
                       std::vector<Node> &nodes);


    // Lookahead related functions
    void get_reverse_arcs(std::vector<std::vector<Arc> > &reverse_arcs);
    void find_successor_words(int node_idx, std::set<int> &word_ids, bool start_node=true);
    void find_predecessor_words(int node_idx, std::set<int> &word_ids,
                                const std::vector<std::vector<Arc> > &reverse_arcs,
                                bool start_node=true);

    // Basic unigram la scores
    int set_unigram_la_scores(); // Returns number of lookahead states
    void propagate_unigram_la_score(int node_idx,
                                    float score,
                                    std::vector<std::vector<Arc> > &reverse_arcs,
                                    int &la_score_set,
                                    bool start_node=true);

    // Bigram scores conditioned on all possible predecessor words
    void set_bigram_la_scores();
    // Bigram la scores to all LM nodes, score conditioned on the subword in the same node
    void set_bigram_la_scores_to_lm_nodes();

    // Number of nodes with branching
    int num_branching_nodes();
    // Sets la state indices to nodes, returns number of la states
    int set_la_state_indices_to_nodes();
    void propagate_la_state_idx(int node_idx,
                                int la_state_idx,
                                bool first_node=true);
    void set_bigram_la_tables();

    // Subwords
    std::vector<std::string> m_subwords;
    // Mapping from text units to indices
    std::map<std::string, int> m_subword_map;
    // Vocabulary units as phone strings, FIXME: currently only one pronunciation
    std::map<std::string, std::vector<std::string> > m_lexicon;
    // Mapping from phones (triphones) to HMM indices.
    std::map<std::string,int> m_hmm_map;
    // The HMMs.
    std::vector<Hmm> m_hmms;
    // Hmm states
    std::vector<HmmState> m_hmm_states;

    // Language model
    Ngram m_lm;
    std::vector<int> m_subword_id_to_ngram_symbol;

    // Lookahead language model
    Ngram m_la_lm;
    std::vector<int> m_subword_id_to_la_ngram_symbol;
    std::vector<std::vector<float> > m_bigram_la_scores;

    // Audio reader
    LnaReaderCircular m_lna_reader;
    Acoustics *m_acoustics;

    std::vector<Node> m_nodes;
    std::set<WordHistory*> m_word_history_leafs;
    std::set<WordHistory*> m_active_histories;

    std::vector<Token> m_raw_tokens;
    std::set<int> m_active_nodes;
    std::vector<std::map<int, Token> > m_recombined_tokens;
    std::vector<float> m_best_node_scores;

    int m_debug;
    int m_stats;

    float m_lm_scale;
    float m_duration_scale;
    float m_transition_scale;
    int m_token_limit;
    int m_active_node_limit;
    int m_token_count;
    int m_propagated_count;
    int m_token_count_after_pruning;
    bool m_force_sentence_end;
    bool m_use_word_boundary_symbol;
    bool m_duration_model_in_use;
    bool m_unigram_la_in_use;
    bool m_bigram_la_in_use;
    bool m_precomputed_lookahead_tables;
    std::string m_word_boundary_symbol;
    int m_word_boundary_symbol_idx;
    int m_sentence_begin_symbol_idx;
    int m_sentence_end_symbol_idx;
    int m_max_state_duration;
    int m_ngram_state_sentence_begin;
    int m_initial_node_depth;

    float m_global_beam;
    float m_acoustic_beam;
    float m_node_beam;
    float m_history_beam;
    float m_silence_beam;
    float m_word_end_beam;
    float m_current_word_end_beam;
    float m_word_boundary_penalty;

    float m_best_log_prob;
    float m_best_am_log_prob;
    float m_best_word_end_prob;
    int m_histogram_bin_limit;

    int m_global_beam_pruned_count;
    int m_acoustic_beam_pruned_count;
    int m_history_beam_pruned_count;
    int m_word_end_beam_pruned_count;
    int m_node_beam_pruned_count;
    int m_max_state_duration_pruned_count;
    int m_histogram_pruned_count;
    int m_dropped_count;

    int m_history_clean_frame_interval;
    WordHistory* m_history_root;

    int m_decode_start_node;
    int m_long_silence_loop_start_node;
    int m_long_silence_loop_end_node;
};

#endif /* DECODER_HH */
