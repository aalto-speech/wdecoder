#ifndef DECODER_HH
#define DECODER_HH

#include <map>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "LM.hh"
#include "LnaReaderCircular.hh"


class Decoder {

public:
    static const int WORD_BOUNDARY_IDENTIFIER;
    int DECODE_START_NODE;
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
              flags(0), unigram_la_score(0.0) { }
        int word_id; // -1 for nodes without word identity.
        int hmm_state; // -1 for nodes without acoustics.
        int flags;
        std::vector<Arc> arcs;
        float unigram_la_score;
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
      int fsa_lm_node;
      unsigned short int dur;
      bool word_end;
      WordHistory *history;

      Token():
        node_idx(-1),
        am_log_prob(0.0f),
        lm_log_prob(0.0f),
        lookahead_log_prob(0.0f),
        total_log_prob(-1e20),
        fsa_lm_node(0),
        dur(0),
        word_end(false),
        history(nullptr)
      { }
    };

    Decoder() {
        m_debug = 0;
        m_stats = 0;
        m_duration_model_in_use = false;
        m_unigram_la_in_use = false;
        m_use_word_boundary_symbol = false;
        m_force_sentence_end = true;

        m_lm_scale = 0.0;
        m_duration_scale = 0.0;
        m_transition_scale = 0.0;
        m_token_count = 0;
        m_propagated_count = 0;
        m_token_count_after_pruning = 0;
        m_word_boundary_symbol_idx = -1;

        m_dropped_count = 0;
        m_global_beam_pruned_count = 0;
        m_word_end_beam_pruned_count = 0;
        m_state_beam_pruned_count = 0;
        m_history_beam_pruned_count = 0;
        m_acoustic_beam_pruned_count = 0;
        m_max_state_duration_pruned_count = 0;

        DECODE_START_NODE = -1;
        SENTENCE_END_WORD_ID = -1;

        m_acoustics = nullptr;

        m_best_log_prob = -1e20;
        m_best_am_log_prob = -1e20;
        m_best_word_end_prob = -1e20;

        m_global_beam = 0.0;
        m_acoustic_beam = 0.0;
        m_current_word_end_beam = 0.0;
        m_history_beam = 0.0;
        m_silence_beam = 0.0;
        m_word_end_beam = 0.0;
        m_state_beam = 0.0;

        m_token_limit = 500000;
        m_active_node_limit = 50000;

        m_history_clean_frame_interval = 10;

        m_word_boundary_penalty = 0.0;
        m_max_state_duration = 80;

        m_track_result = false;
    };

    void read_phone_model(std::string phnfname);
    void read_duration_model(std::string durfname);
    void read_noway_lexicon(std::string lexfname);
    void read_lm(std::string lmfname);
    void read_la_lm(std::string lmfname);
    void read_dgraph(std::string graphfname);

    void read_config(std::string cfgfname);
    void print_config(std::ostream &outf);

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
    void print_best_word_history(std::ostream &outf=std::cout);
    void word_history_to_string(WordHistory *history,
                                std::string &whs);
    void print_word_history(WordHistory *history,
                            std::ostream &outf=std::cout,
                            bool print_lm_probs=false);
    void print_dot_digraph(std::vector<Node> &nodes, std::ostream &fstr);

    void find_successor_words(int node_idx, std::map<int, std::set<int> > &word_ids, bool start_node=false);
    void set_unigram_la_scores();

    float score_state_path(std::string lnafname,
                           std::string sfname,
                           bool duration_model=true);

    void set_tracked_result(std::string result);
    int recombined_to_vector(std::vector<Token> &raw_tokens);
    bool track_result(std::vector<Token> &tokens);

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
    fsalm::LM m_lm;
    std::vector<int> m_subword_id_to_fsa_symbol;

    // Lookahead language model
    fsalm::LM m_la_lm;
    std::vector<int> m_subword_id_to_la_fsa_symbol;

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

private:

    void add_silence_hmms(std::vector<Node> &nodes,
                          bool long_silence=true,
                          bool short_silence=false);
    void set_hmm_transition_probs(std::vector<Node> &nodes);
    void set_subword_id_fsa_symbol_mapping();
    void set_subword_id_la_fsa_symbol_mapping();
    void clear_word_history();
    void prune_word_history();
    void set_word_boundaries();
    void active_nodes_sorted_by_best_lp(std::vector<int> &nodes);
    void reset_history_scores();

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
    std::string m_word_boundary_symbol;
    int m_word_boundary_symbol_idx;
    int m_max_state_duration;

    float m_global_beam;
    float m_acoustic_beam;
    float m_state_beam;
    float m_history_beam;
    float m_silence_beam;
    float m_word_end_beam;
    float m_current_word_end_beam;
    float m_word_boundary_penalty;

    float m_best_log_prob;
    float m_best_am_log_prob;
    float m_best_word_end_prob;

    int m_global_beam_pruned_count;
    int m_acoustic_beam_pruned_count;
    int m_history_beam_pruned_count;
    int m_word_end_beam_pruned_count;
    int m_state_beam_pruned_count;
    int m_max_state_duration_pruned_count;
    int m_dropped_count;

    int m_history_clean_frame_interval;
    WordHistory* m_empty_history;

    bool m_track_result;
    std::vector<int> m_tracked_result;
    std::set<WordHistory*> m_tracked_histories;
};

#endif /* DECODER_HH */
