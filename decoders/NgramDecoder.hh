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
    class NgramToken : public Token {
    public:
        int lm_node;
        NgramToken(): lm_node(0) { };
    };

    NgramRecognition(NgramDecoder &decoder);
    void recognize_lna_file(std::string lnafname,
                            RecognitionResult &res);
private:
    void reset_frame_variables();
    void propagate_tokens();
    void prune_tokens(bool collect_active_histories=false);
    void move_token_to_node(NgramToken token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead);
    virtual void get_tokens(std::vector<Token*> &tokens);
    virtual void add_sentence_ends(std::vector<Token*> &tokens);
    std::string get_best_word_history();
    std::string get_word_history(WordHistory *history);

    int m_ngram_state_sentence_begin;
    std::vector<NgramToken> m_raw_tokens;
    std::vector<std::map<int, NgramToken> > m_recombined_tokens;
    NgramDecoder *ngd;
};

#endif /* NGRAM_DECODER_HH */
