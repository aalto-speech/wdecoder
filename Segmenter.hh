#include "Decoder.hh"


class Segmenter : public Decoder {
public:

    Segmenter();

    class StateHistory {
    public:
        StateHistory()
            : hmm_state(-1),
              start_frame(-1), end_frame(-1) { }
        StateHistory(int hmm_state)
            : hmm_state(hmm_state),
              start_frame(-1), end_frame(-1) { }
        int hmm_state;
        int start_frame;
        int end_frame;
        std::string label;
    };

    class SToken : public Token {
    public:
        std::vector<StateHistory> state_history;
        SToken() { };
    };

    void initialize();

    void segment_lna_file(std::string lnafname,
                          std::map<int, std::string> &node_labels,
                          std::ostream &outf=std::cout);

    void print_phn_segmentation(SToken &token,
                                std::ostream &outf=std::cout);

    void advance_in_state_history(SToken& token);

    void move_token_to_node(SToken token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead);
    void propagate_tokens();
    void recombine_tokens();

    std::map<int, std::string> m_state_history_labels;
    std::vector<SToken> m_raw_tokens;
    std::vector<SToken> m_recombined_tokens;
};
