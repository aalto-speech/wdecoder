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

    class Lookahead {
    public:
        virtual float get_lookahead_score(int node_idx, int word_id) = 0;
        void set_text_unit_id_la_ngram_symbol_mapping();
        void find_successor_words(int node_idx, std::vector<int> &word_ids);
        void find_successor_words(int node_idx, std::set<int> &word_ids, bool start_node=true);
        void find_predecessor_words(int node_idx,
                                    std::set<int> &word_ids,
                                    const std::vector<std::vector<Arc> > &reverse_arcs);
        bool detect_one_predecessor_node(int node_idx,
                                         int &predecessor_count,
                                         const std::vector<std::vector<Arc> > &reverse_arcs);
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
    void get_reverse_arcs(std::vector<std::vector<Arc> > &reverse_arcs);
    void mark_initial_nodes(int max_depth, int curr_depth=0, int node=START_NODE);
    void mark_tail_nodes(int max_depth,
                         std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                         int curr_depth=0,
                         int node=END_NODE);
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

class RecognitionResult {
public:
    RecognitionResult();
    void add_result(std::string result,
                    double total_lp,
                    double total_am_lp,
                    double total_lm_lp);
    std::string get_best_result();
    void print_file_stats(std::ostream &statsf);

    class Result {
    public:
        Result() : result(""), total_lp(0.0), total_am_lp(0.0), total_lm_lp(0.0) { };
        std::string result;
        double total_lp;
        double total_am_lp;
        double total_lm_lp;
    };

    long long int total_frames;
    double total_time;
    double total_token_count;
    std::multimap<double, Result> results;
};

class TotalRecognitionStats {
public:
    TotalRecognitionStats() :
        num_files(0),
        total_frames(0), total_time(0.0), total_token_count(0.0),
        total_lp(0.0), total_am_lp(0.0), total_lm_lp(0.0) { };
    void accumulate(RecognitionResult &res);
    void print_stats(std::ostream &statsf);

    int num_files;
    long long int total_frames;
    double total_time;
    double total_token_count;
    double total_lp;
    double total_am_lp;
    double total_lm_lp;
};

class Recognition {
public:
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
        int last_word_id;
        WordHistory *history;
        unsigned short int dur;
        bool word_end;
        int histogram_bin;
        Decoder *d;

        Token():
            node_idx(-1),
            am_log_prob(0.0f),
            lm_log_prob(0.0f),
            lookahead_log_prob(0.0f),
            total_log_prob(-1e20),
            last_word_id(-1),
            history(nullptr),
            dur(0),
            word_end(false),
            histogram_bin(0),
            d(nullptr)
        { }
        void update_total_log_prob();
        void apply_duration_model();
        void update_lookahead_prob(float lookahead_prob);
    };

    Recognition(Decoder &decoder);
    virtual ~Recognition() { };
    void recognize_lna_file(std::string lnafname,
                            RecognitionResult &res);
    void prune_word_history();
    void clear_word_history();
    void print_certain_word_history(std::ostream &outf=std::cout);
    void advance_in_word_history(Token *token, int word_id);
    virtual void get_tokens(std::vector<Token*> &tokens) = 0;
    virtual void add_sentence_ends(std::vector<Token*> &tokens) = 0;
    Token* get_best_token(std::vector<Token*> &tokens);
    Token* get_best_end_token(std::vector<Token*> &tokens);

protected:
    virtual void reset_frame_variables() = 0;
    virtual void propagate_tokens() = 0;
    virtual std::string get_best_word_history() = 0;
    virtual std::string get_word_history(WordHistory *history) = 0;
    virtual void prune_tokens(bool collect_active_histories=false) = 0;

    LnaReaderCircular m_lna_reader;
    Acoustics *m_acoustics;

    WordHistory* m_history_root;
    std::set<WordHistory*> m_word_history_leafs;
    std::set<WordHistory*> m_active_histories;

    std::set<int> m_active_nodes;
    std::vector<float> m_best_node_scores;

    // Passed from Decoder
    int m_stats;
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
    Decoder *d;
};

#endif /* DECODER_HH */
