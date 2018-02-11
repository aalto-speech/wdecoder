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
    ClassDecoder();
    ~ClassDecoder();
    void read_class_lm(
            std::string ngramfname,
            std::string wordpfname);
    int read_class_memberships(
            std::string fname,
            std::map<std::string, std::pair<int, float> > &class_memberships);

    LNNgram m_class_lm;
    std::map<std::string, std::pair<int, float> > m_class_memberships;
    std::vector<std::pair<int, float> > m_class_membership_lookup;
    std::vector<int> m_class_intmap;
};


class ClassRecognition : public Recognition {
public:
    class ClassToken : public Token {
    public:
        int class_lm_node;
        ClassToken(): class_lm_node(0) { };
    };

    ClassRecognition(ClassDecoder &decoder);

    virtual void reset_frame_variables();
    virtual void propagate_tokens();
    virtual void prune_tokens(bool collect_active_histories=false);
    void move_token_to_node(ClassToken token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead);
    bool update_lm_prob(ClassToken &token, int node_idx);
    double class_lm_score(ClassToken &token, int word_id);
    virtual void get_tokens(std::vector<Token*> &tokens);
    virtual void add_sentence_ends(std::vector<Token*> &tokens);
    virtual std::string get_best_word_history();
    virtual std::string get_word_history(WordHistory *history);

    std::vector<ClassToken> m_raw_tokens;
    std::vector<std::map<int, ClassToken> > m_recombined_tokens;
    ClassDecoder *cld;
};

#endif /* CLASS_DECODER_HH */
