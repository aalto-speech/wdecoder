#ifndef NGRAM_DECODER_HH
#define NGRAM_DECODER_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "Ngram.hh"
#include "LnaReaderCircular.hh"
#include "Decoder.hh"


class NgramDecoder : public Decoder {
public:
    NgramDecoder();
    ~NgramDecoder();
    void read_lm(std::string lmfname);
    void set_text_unit_id_ngram_symbol_mapping();

    // Language model
    Ngram m_lm;
    std::vector<int> m_text_unit_id_to_ngram_symbol;
    int m_ngram_state_sentence_begin;
};

class NgramRecognition : public Recognition {
public:
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

    NgramRecognition(NgramDecoder &decoder);
    void recognize_lna_file(std::string lnafname,
                            std::ostream &outf=std::cout,
                            int *frame_count=nullptr,
                            double *seconds=nullptr,
                            double *log_prob=nullptr,
                            double *am_prob=nullptr,
                            double *lm_prob=nullptr,
                            double *total_token_count=nullptr);
private:
    void reset_frame_variables();
    void update_total_log_prob(Token &token) const;
    void apply_duration_model(Token &token, int node_idx) const;
    void update_lookahead_prob(Token &token, float lookahead_prob) const;
    void propagate_tokens();
    void prune_tokens(bool collect_active_histories=false);
    void move_token_to_node(Token token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead);
    void advance_in_word_history(Token& token, int word_id);
    Token* get_best_token();
    Token* get_best_token(std::vector<Token> &tokens);
    Token* get_best_end_token(std::vector<Token> &tokens);
    void add_sentence_ends(std::vector<Token> &tokens);
    void print_best_word_history(std::ostream &outf=std::cout);
    void print_word_history(WordHistory *history,
                            std::ostream &outf=std::cout,
                            bool print_lm_probs=false);

    int m_ngram_state_sentence_begin;
    std::vector<Token> m_raw_tokens;
    std::vector<std::map<int, Token> > m_recombined_tokens;
    NgramDecoder *d;
};

#endif /* NGRAM_DECODER_HH */
