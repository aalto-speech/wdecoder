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
    class WSWToken : public Token {
    public:
        int lm_node;
        int class_lm_node;
        int subword_lm_node;
        WSWToken():
            lm_node(0),
            class_lm_node(0),
            subword_lm_node(0) { }
    };

    WordSubwordRecognition(WordSubwordDecoder &decoder);
    void recognize_lna_file(std::string lnafname,
                            RecognitionResult &res);

    void reset_frame_variables();
    void propagate_tokens();
    void prune_tokens(bool collect_active_histories=false);
    void move_token_to_node(WSWToken token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead);
    bool update_lm_prob(WSWToken &token, int node_idx);
    void advance_in_word_history(WSWToken& token, int word_id);
    double class_lm_score(WSWToken &token, int word_id);
    WSWToken* get_best_token();
    WSWToken* get_best_token(std::vector<WSWToken> &tokens);
    WSWToken* get_best_end_token(std::vector<WSWToken> &tokens);
    void add_sentence_ends(std::vector<WSWToken> &tokens);
    std::string get_best_word_history();
    std::string get_word_history(WordHistory *history);

    std::vector<WSWToken> m_raw_tokens;
    std::vector<std::map<std::pair<int, int>, WSWToken> > m_recombined_tokens;
    WordSubwordDecoder *wswd;
};

#endif /* WORD_SUBWORD_DECODER_HH */
