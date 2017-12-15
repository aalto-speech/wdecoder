#include <map>

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

    class Token {
    public:
        std::vector<StateHistory> state_history;
        int node_idx;
        float log_prob;
        unsigned short int dur;
        int histogram_bin;

        Token():
            node_idx(-1),
            log_prob(0.0),
            dur(0),
            histogram_bin(0)
        { }
    };

    void initialize();
    bool segment_lna_file(std::string lnafname,
                          std::map<int, std::string> &node_labels,
                          std::ostream &outf=std::cout);
    void apply_duration_model(Token &token, int node_idx);
    void print_phn_segmentation(Token &token,
                                std::ostream &outf=std::cout);
    void advance_in_state_history(Token& token);
    void move_token_to_node(Token token,
                            int node_idx,
                            float transition_score);
    void active_nodes_sorted_by_lp(std::vector<int> &nodes);
    void propagate_tokens();
    void recombine_tokens();

    std::map<int, std::string> m_state_history_labels;
    std::vector<Token> m_raw_tokens;
    std::map<int, Token> m_recombined_tokens;
    std::set<int> m_active_nodes;

    int m_decode_end_node;
    int m_frame_idx;
    int m_global_beam_pruned_count;
    float m_best_log_prob;
    LnaReaderCircular m_lna_reader;
    Acoustics *m_acoustics;

};
