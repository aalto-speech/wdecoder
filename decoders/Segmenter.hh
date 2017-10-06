#include "Decoder.hh"


class Segmenter : public Decoder {
public:

    Segmenter();

    class StateHistory {
    public:
        StateHistory()
            : node_idx(-1),
              start_frame(-1), end_frame(-1) { }
        StateHistory(int node_idx)
            : node_idx(node_idx),
              start_frame(-1), end_frame(-1) { }
        int node_idx;
        int start_frame;
        int end_frame;
    };

    class SToken : public Token {
    public:
        std::vector<StateHistory> state_history;
        SToken() { };
    };

    void initialize();

    bool segment_lna_file(std::string lnafname,
                          std::map<int, std::string> &node_labels,
                          std::ostream &outf=std::cout);

    void print_phn_segmentation(SToken &token,
                                std::ostream &outf=std::cout);

    void advance_in_state_history(SToken& token);

    void move_token_to_node(SToken token,
                            int node_idx,
                            float transition_score);
    void active_nodes_sorted_by_lp(std::vector<int> &nodes);
    void propagate_tokens();
    void recombine_tokens();

    std::map<int, std::string> m_state_history_labels;
    std::vector<SToken> m_raw_tokens;
    std::vector<SToken> m_recombined_tokens;

    int m_decode_end_node;
};
