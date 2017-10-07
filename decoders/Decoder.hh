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
    void read_lm(std::string lmfname);
    void read_dgraph(std::string graphfname);

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
    void update_total_log_prob(Token &token);
    void advance_in_word_history(Token& token, int word_id);
    void apply_duration_model(Token &token, int node_idx);
    void update_lookahead_prob(Token &token, float lookahead_prob);
    Token* get_best_token();
    Token* get_best_token(std::vector<Token> &tokens);
    Token* get_best_end_token(std::vector<Token> &tokens);
    void add_sentence_ends(std::vector<Token> &tokens);
    void print_certain_word_history(std::ostream &outf=std::cout);
    void print_best_word_history(std::ostream &outf=std::cout);
    void print_word_history(WordHistory *history,
                            std::ostream &outf=std::cout,
                            bool print_lm_probs=false);

    void print_dot_digraph(std::vector<Node> &nodes, std::ostream &fstr);
    void set_text_unit_id_ngram_symbol_mapping();

    void prune_word_history();
    void clear_word_history();

    void set_hmm_transition_probs();
    void active_nodes_sorted_by_best_lp(std::vector<int> &nodes);

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

    // Language model
    Ngram m_lm;
    std::vector<int> m_text_unit_id_to_ngram_symbol;

    // Lookahead language model
    Lookahead *m_la;

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

#endif /* DECODER_HH */
