#ifndef CLASS_DECODER_HH
#define CLASS_DECODER_HH

#include <array>
#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "Ngram.hh"
#include "LnaReaderCircular.hh"
#include "Decoder.hh"


class ClassDecoder : public Decoder {

public:

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
    int read_class_memberships(std::string fname,
                               std::map<std::string, std::pair<int, float> > &class_memberships);

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
    void print_best_word_history(std::ostream &outf=std::cout);
    void print_word_history(WordHistory *history,
                            std::ostream &outf=std::cout,
                            bool print_lm_probs=false);

    // Class n-gram language model
    LNNgram m_class_lm;
    std::map<std::string, std::pair<int, float> > m_class_memberships;
    std::vector<std::pair<int, float> > m_class_membership_lookup;
    std::vector<int> m_class_intmap;

    std::vector<Token> m_raw_tokens;
    std::vector<std::map<int, Token> > m_recombined_tokens;
};

#endif /* CLASS_DECODER_HH */
