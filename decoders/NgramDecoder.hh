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

private:
    void reset_frame_variables() override;
    void propagate_tokens() override;
    void prune_tokens(bool collect_active_histories=false,
                      bool write_nbest=false) override;
    void move_token_to_node(NgramToken token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead);
    void get_tokens(std::vector<Token*> &tokens) override;
    void add_sentence_ends(std::vector<Token*> &tokens) override;
    std::string get_result(WordHistory *history) override;

    int m_ngram_state_sentence_begin;
    std::vector<NgramToken> m_raw_tokens;
    std::vector<std::map<int, NgramToken> > m_recombined_tokens;
    NgramDecoder *ngd;
};

#endif /* NGRAM_DECODER_HH */
