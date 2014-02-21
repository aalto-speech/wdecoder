#ifndef DECODER_HH
#define DECODER_HH

#include <map>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <vector>
#include <set>
#include <deque>

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
        Node() : word_id(-1), hmm_state(-1), flags(0) { }
        int word_id; // -1 for nodes without word identity.
        int hmm_state; // -1 for nodes without acoustics.
        int flags;
        std::vector<Arc> arcs;
    };

    class Token;

    class WordHistory {
    public:
        WordHistory() : word_id(-1), previous(nullptr), best_total_log_prob(-1e20),
                        best_am_log_prob(-1e20), tokens(nullptr) { }
        WordHistory(int word_id, WordHistory *previous,
                    float best_token_score=-1e20)
            : word_id(word_id), previous(previous),
              best_am_log_prob(best_token_score), best_total_log_prob(-1e20),
              tokens(nullptr) { }
        int word_id;
        WordHistory *previous;
        //std::unordered_map<int, WordHistory*> next;
        std::map<int, WordHistory*> next;
        float best_am_log_prob;
        float best_total_log_prob;
        std::map<int, Decoder::Token> *tokens;
    };

    class Token {
    public:
      int node_idx;
      float am_log_prob;
      float cur_am_log_prob;
      float lm_log_prob;
      float total_log_prob;
      int fsa_lm_node;
      int word_start_frame;
      int word_count;
      unsigned short int dur;
      bool word_end;
      WordHistory *history;

      Token():
        node_idx(-1),
        am_log_prob(0.0f),
        cur_am_log_prob(0.0f),
        lm_log_prob(0.0f),
        total_log_prob(-1e20),
        fsa_lm_node(0),
        word_start_frame(0),
        word_count(0),
        dur(0)
      { }
    };

    Decoder() {
        m_debug = 0;
        m_stats = 0;
        m_duration_model_in_use = false;
        m_use_word_boundary_symbol = false;
        m_force_sentence_end = true;

        m_lm_scale = 0.0;
        m_duration_scale = 0.0;
        m_transition_scale = 0.0;
        m_histogram_prune_trigger = 0;
        m_histogram_prune_target = 0;
        m_token_count = 0;
        m_propagated_count = 0;
        m_token_count_after_pruning = 0;

        m_global_beam = 0.0;
        m_acoustic_beam = 0.0;
        m_current_word_end_beam = 0.0;
        m_history_beam = 0.0;
        m_silence_beam = 0.0;
        m_word_end_beam = 0.0;
        m_state_beam = 0.0;

        m_history_limit = 5000;

        m_history_clean_frame_interval = 10;
        m_empty_history = nullptr;

        m_word_boundary_penalty = 0.0;
    };

    void read_phone_model(std::string phnfname);
    void read_duration_model(std::string durfname);
    void read_noway_lexicon(std::string lexfname);
    void read_lm(std::string lmfname);
    void read_dgraph(std::string graphfname);

    void read_config(std::string cfgfname);
    void print_config(std::ostream &outf);

    void set_lm_scale(float lm_scale) { m_lm_scale = lm_scale; }
    void set_duration_scale(float dur_scale) { m_duration_scale = dur_scale; }
    void set_transition_scale(float trans_scale) { m_transition_scale = trans_scale; }
    void set_histogram_prune_trigger(int tokens) { m_histogram_prune_trigger = tokens; }
    void set_histogram_prune_target(int tokens) { m_histogram_prune_target = tokens; }
    void set_history_limit(int histories) { m_history_limit = histories; }
    void set_word_boundary_penalty(float penalty) { m_word_boundary_penalty = penalty; }
    void set_state_beam(float beam) { m_state_beam = beam; }
    void set_global_beam(float beam) { m_global_beam = beam; }
    void set_history_beam(float beam) { m_history_beam = beam; }
    void set_silence_beam(float beam) { m_silence_beam = beam; }
    void set_word_end_beam(float beam) { m_word_end_beam = beam; }
    void set_force_sentence_end(bool force) { m_force_sentence_end = force; }
    void set_use_word_boundary_symbol(std::string symbol) {
        m_word_boundary_symbol = symbol;
        m_use_word_boundary_symbol = true;
    }

    void recognize_lna_file(std::string lnafname,
                            std::ostream &outf=std::cout,
                            int *frame_count=nullptr,
                            double *seconds=nullptr,
                            double *log_prob=nullptr,
                            double *am_prob=nullptr,
                            double *lm_prob=nullptr);
    void initialize();
    void propagate_tokens();
    void prune_tokens();
    void move_token_to_node(Token token, int node_idx, float transition_score);
    inline float get_token_log_prob(float am_score, float lm_score)
    {
        return (am_score + m_lm_scale * lm_score);
    }
    Token* get_best_token();
    Token get_best_token(std::vector<Token> &tokens);
    inline void advance_in_history(Token& token, int word_id);
    void add_sentence_ends(std::vector<Token> &tokens);
    void print_best_word_history(std::ostream &outf=std::cout);
    void print_word_history(WordHistory *history,
                            std::ostream &outf=std::cout,
                            bool print_lm_probs=false);
    void print_dot_digraph(std::vector<Node> &nodes, std::ostream &fstr);
    void sort_histories_by_best_lp(const std::set<WordHistory*> &histories,
                                   std::vector<WordHistory*> &sorted_histories);

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

    // Audio reader
    LnaReaderCircular m_lna_reader;
    Acoustics *m_acoustics;

    std::vector<Node> m_nodes;
    std::set<WordHistory*> m_word_history_leafs;
    //std::unordered_map<int, std::unordered_map<WordHistory*, Token> > m_tokens;
    std::vector<Token> m_tokens;
    std::vector<Token> m_raw_tokens;
    std::set<WordHistory*> m_active_histories;

private:

    void add_silence_hmms(std::vector<Node> &nodes,
                          bool long_silence=true,
                          bool short_silence=false);
    void set_hmm_transition_probs(std::vector<Node> &nodes);
    void set_subword_id_fsa_symbol_mapping();
    void clear_word_history();
    void reset_history_scores();
    void prune_word_history();
    void set_word_boundaries();
    void apply_duration_model(Token &token, int node_idx);

    int m_debug;
    int m_stats;

    float m_lm_scale;
    float m_duration_scale;
    float m_transition_scale; // Temporary scaling used for self transitions
    int m_histogram_prune_trigger;
    int m_histogram_prune_target;
    int m_history_limit;
    int m_token_count;
    int m_propagated_count;
    int m_token_count_after_pruning;
    bool m_force_sentence_end;
    bool m_use_word_boundary_symbol;
    bool m_duration_model_in_use;
    std::string m_word_boundary_symbol;

    float m_global_beam;
    float m_acoustic_beam;
    float m_state_beam;
    float m_history_beam;
    float m_silence_beam;
    float m_word_end_beam;
    float m_current_word_end_beam;
    float m_word_boundary_penalty;

    float m_best_log_prob;
    int m_best_node_idx;
    WordHistory *m_best_history;
    float m_best_am_log_prob;
    float m_worst_log_prob;
    float m_best_word_end_prob;

    int m_global_beam_pruned_count;
    int m_acoustic_beam_pruned_count;
    int m_history_beam_pruned_count;
    int m_histogram_pruned_count;
    int m_word_end_beam_pruned_count;
    int m_state_beam_pruned_count;
    int m_dropped_count;

    int m_history_clean_frame_interval;
    WordHistory* m_empty_history;
};

#endif /* DECODER_HH */
