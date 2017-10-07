#ifndef CLASS_DECODER_HH
#define CLASS_DECODER_HH

#include <array>
#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "Decoder.hh"
#include "defs.hh"
#include "Hmm.hh"
#include "Ngram.hh"
#include "LnaReaderCircular.hh"

#define HISTOGRAM_BIN_COUNT 100


class ClassDecoder : public Decoder {

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

    class Token;

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

    class Token {
    public:
        int node_idx;
        float am_log_prob;
        float lm_log_prob;
        float lookahead_log_prob;
        float total_log_prob;
        int class_lm_node;
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
            class_lm_node(0),
            last_word_id(-1),
            history(nullptr),
            dur(0),
            word_end(false),
            histogram_bin(0)
        { }
    };

    ClassDecoder();
    ~ClassDecoder();

    void read_class_lm(std::string ngramfname,
                       std::string wordpfname);

    void recognize_lna_file(std::string lnafname,
                            std::ostream &outf=std::cout,
                            int *frame_count=nullptr,
                            double *seconds=nullptr,
                            double *log_prob=nullptr,
                            double *am_prob=nullptr,
                            double *lm_prob=nullptr,
                            double *total_token_count=nullptr);

    void initialize();
    void reset_frame_variables();
    void propagate_tokens();
    void prune_tokens(bool collect_active_histories=false);
    void move_token_to_node(Token token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead);
    bool update_lm_prob(Token &token, int node_idx);
    void update_total_log_prob(Token &token);
    void advance_in_word_history(Token& token, int word_id);
    void apply_duration_model(Token &token, int node_idx);
    void update_lookahead_prob(Token &token, float lookahead_prob);
    double class_lm_score(Token &token, int word_id);
    Token* get_best_token();
    Token* get_best_token(std::vector<Token> &tokens);
    Token* get_best_end_token(std::vector<Token> &tokens);
    void add_sentence_ends(std::vector<Token> &tokens);
    void print_certain_word_history(std::ostream &outf=std::cout);
    void print_best_word_history(std::ostream &outf=std::cout);
    void print_word_history(WordHistory *history,
                            std::ostream &outf=std::cout,
                            bool print_lm_probs=false);

    void set_subword_id_ngram_symbol_mapping();

    void prune_word_history();
    void clear_word_history();

    void active_nodes_sorted_by_best_lp(std::vector<int> &nodes);

    // Lookahead language model
    Lookahead *m_la;

    // Class n-gram language model
    LNNgram m_class_lm;
    std::map<std::string, std::pair<int, float> > m_class_memberships;
    std::vector<std::pair<int, float> > m_class_membership_lookup;
    std::vector<int> m_class_intmap;

    // Audio reader
    LnaReaderCircular m_lna_reader;
    Acoustics *m_acoustics;

    std::vector<Node> m_nodes;

    WordHistory* m_history_root;
    std::set<WordHistory*> m_word_history_leafs;
    std::set<WordHistory*> m_active_histories;

    std::vector<Token> m_raw_tokens;
    std::set<int> m_active_nodes;
    std::vector<std::map<int, Token> > m_recombined_tokens;
    std::vector<float> m_best_node_scores;

    int m_stats;

    double m_total_token_count;

    float m_lm_scale;
    float m_duration_scale;
    float m_transition_scale;
    int m_token_limit;
    int m_token_count;
    int m_token_count_after_pruning;
    bool m_force_sentence_end;
    bool m_use_word_boundary_symbol;
    bool m_duration_model_in_use;
    std::string m_word_boundary_symbol;
    int m_word_boundary_symbol_idx;
    int m_word_boundary_symbol_class_idx;
    int m_sentence_begin_symbol_idx;
    int m_sentence_end_symbol_idx;
    int m_max_state_duration;
    int m_ngram_state_sentence_begin;

    float m_global_beam;
    float m_node_beam;
    float m_word_end_beam;

    float m_best_log_prob;
    float m_best_word_end_prob;
    int m_histogram_bin_limit;

    int m_global_beam_pruned_count;
    int m_word_end_beam_pruned_count;
    int m_node_beam_pruned_count;
    int m_max_state_duration_pruned_count;
    int m_histogram_pruned_count;
    int m_dropped_count;

    int m_history_clean_frame_interval;

    int m_decode_start_node;
    int m_frame_idx;

    int m_last_sil_idx;
};

#endif /* CLASS_DECODER_HH */
