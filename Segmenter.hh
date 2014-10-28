#include "Decoder.hh"


class Segmenter : public Decoder {
public:

    Segmenter();

    class StateHistory {
    public:
        StateHistory()
            : hmm_state(-1), previous(nullptr),
              start_frame(-1), end_frame(-1),
              best_am_log_prob(-1e20) { }
        StateHistory(int hmm_state, StateHistory *previous)
            : hmm_state(hmm_state), previous(previous),
              start_frame(-1), end_frame(-1),
              best_am_log_prob(-1e20) { }
        int hmm_state;
        StateHistory *previous;
        std::map<int, StateHistory*> next;
        int start_frame;
        int end_frame;
        float best_am_log_prob;
        std::string label;
    };

    class SToken : public Token {
    public:
        StateHistory *state_history;
        SToken(): state_history(nullptr) { }
    };

    void initialize();

    void segment_lna_file(std::string lnafname,
                          std::map<int, std::string> &node_labels,
                          std::ostream &outf=std::cout);

    void print_phn_segmentation(StateHistory *history,
                                std::ostream &outf=std::cout);

    void advance_in_state_history(SToken& token);

    void prune_state_history();
    void clear_state_history();

    void move_token_to_node(SToken token,
                            int node_idx,
                            float transition_score,
                            bool update_lookahead);
    void propagate_tokens();
    void recombine_tokens();

    StateHistory* m_state_history_root;
    std::set<StateHistory*> m_state_history_leafs;
    std::set<StateHistory*> m_active_state_histories;
    std::map<int, std::string> m_state_history_labels;
    std::vector<SToken> m_raw_tokens;
    std::vector<std::map<int, SToken> > m_recombined_tokens;
};
