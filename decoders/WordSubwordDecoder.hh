#ifndef WORD_SUBWORD_DECODER_HH
#define WORD_SUBWORD_DECODER_HH

#include <array>
#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "Ngram.hh"
#include "Decoder.hh"
#include "LnaReaderCircular.hh"


class WordSubwordDecoder : public Decoder {
public:
    WordSubwordDecoder();
    ~WordSubwordDecoder();

    void read_lm(std::string lmfname);
    void set_text_unit_id_ngram_symbol_mapping();
    void read_class_lm(
        std::string ngramfname,
        std::string wordpfname);
    int read_class_memberships(
        std::string fname,
        std::map<std::string, std::pair<int, float> > &class_memberships);
    void read_subword_lm(
        std::string ngramfname,
        std::string segfname);

    // Language model
    LNNgram m_lm;
    std::vector<int> m_text_unit_id_to_ngram_symbol;

    // Class n-gram language model
    LNNgram m_class_lm;
    std::map<std::string, std::pair<int, float> > m_class_memberships;
    std::vector<std::pair<int, float> > m_class_membership_lookup;
    std::vector<int> m_class_intmap;

    // Subword language model
    LNNgram m_subword_lm;
    std::vector<std::vector<int> > m_word_id_to_subword_ngram_symbols;
    int m_subword_lm_start_node;

    double m_word_iw;
    double m_class_iw;
    double m_subword_iw;
};


class WordSubwordRecognition : public Recognition {
public:
    class Token {
    public:
        int node_idx;
        float am_log_prob;
        float lm_log_prob;
        float lookahead_log_prob;
        float total_log_prob;
        int lm_node;
        int class_lm_node;
        int subword_lm_node;
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
            class_lm_node(0),
            subword_lm_node(0),
            last_word_id(-1),
            history(nullptr),
            dur(0),
            word_end(false),
            histogram_bin(0)
        { }
    };

    WordSubwordRecognition(WordSubwordDecoder &decoder);
    void recognize_lna_file(std::string lnafname,
                            RecognitionResult &res);

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
    std::string get_best_word_history();
    std::string get_word_history(WordHistory *history);

    std::vector<Token> m_raw_tokens;
    std::vector<std::map<std::pair<int, int>, Token> > m_recombined_tokens;
    WordSubwordDecoder *d;
};

#endif /* WORD_SUBWORD_DECODER_HH */
