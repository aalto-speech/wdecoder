#ifndef CLASS_INTERPOLATED_DECODER_HH
#define CLASS_INTERPOLATED_DECODER_HH

#include <array>
#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "Decoder.hh"
#include "Lookahead.hh"
#include "defs.hh"
#include "Hmm.hh"
#include "Ngram.hh"
#include "LnaReaderCircular.hh"


class ClassIPDecoder : public Decoder {
public:
    ClassIPDecoder();
    ~ClassIPDecoder();
    void read_lm(std::string lmfname);
    void set_text_unit_id_ngram_symbol_mapping();
    void read_class_lm(std::string ngramfname,
                       std::string wordpfname);
    int read_class_memberships(std::string fname,
                               std::map<std::string, std::pair<int, float> > &class_memberships);

    // N-gram language model
    LNNgram m_lm;
    std::vector<int> m_text_unit_id_to_ngram_symbol;

    // Class n-gram language model
    LNNgram m_class_lm;
    std::map<std::string, std::pair<int, float> > m_class_memberships;
    std::vector<std::pair<int, float> > m_class_membership_lookup;
    std::vector<int> m_class_intmap;
    double m_iw;
    double m_word_iw;
    double m_class_iw;
};

class ClassIPRecognition : public Recognition {
public:
    class ClassIPToken : public Token {
    public:
        int lm_node;
        int class_lm_node;
        ClassIPToken():
            lm_node(0),
            class_lm_node(0) { }
    };

    ClassIPRecognition(ClassIPDecoder &decoder);
    void recognize_lna_file(std::string lnafname,
                            RecognitionResult &res);

    void reset_frame_variables();
    void propagate_tokens();
    void prune_tokens(bool collect_active_histories=false);
    void move_token_to_node(ClassIPToken token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead);
    bool update_lm_prob(ClassIPToken &token, int node_idx);
    void advance_in_word_history(ClassIPToken& token, int word_id);
    double class_lm_score(ClassIPToken &token, int word_id);
    ClassIPToken* get_best_token();
    ClassIPToken* get_best_token(std::vector<ClassIPToken> &tokens);
    ClassIPToken* get_best_end_token(std::vector<ClassIPToken> &tokens);
    void add_sentence_ends(std::vector<ClassIPToken> &tokens);
    void print_best_word_history(std::ostream &outf=std::cout);
    void print_word_history(WordHistory *history,
                            std::ostream &outf=std::cout,
                            bool print_lm_probs=false);
    std::string get_best_word_history();
    std::string get_word_history(WordHistory *history);

    std::vector<ClassIPToken> m_raw_tokens;
    std::vector<std::map<std::pair<int, int>, ClassIPToken> > m_recombined_tokens;
    ClassIPDecoder *cid;
};

#endif /* CLASS_INTERPOLATED_DECODER_HH */
