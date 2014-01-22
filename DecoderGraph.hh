#ifndef DECODERGRAPH_HH
#define DECODERGRAPH_HH

#include <map>
#include <vector>
#include <set>

#include "Hmm.hh"


class DecoderGraph {

public:
    struct SubwordNode {
        int subword_id;
        std::vector<std::pair<int, int> > in_arcs;  // subword id (lookahead), node id
        std::map<int, int> out_arcs;                // subword id (lookahead), node id
    };
    static const int START_NODE = 0;
    static const int END_NODE = 1;

    class Arc {
    public:
        Arc() : log_prob(0.0), target_node(-1) { }
        float log_prob;
        int target_node;
    };

    class Node {
    public:
        Node() : word_id(-1), hmm_state(-1) { }
        int word_id; // -1 for nodes without word identity.
        int hmm_state; // -1 for nodes without acoustics.
        std::vector<Arc> arcs;
        std::vector<Arc> reverse_arcs;
    };

    DecoderGraph() { };

    void read_phone_model(std::string phnfname);
    void read_duration_model(std::string durfname);
    void read_noway_lexicon(std::string lexfname);
    void read_word_segmentations(std::string segfname);

    void create_word_graph(std::vector<SubwordNode> &nodes);
    void tie_subword_suffixes(std::vector<SubwordNode> &nodes, int min_length=2);
    void print_word_graph(std::vector<SubwordNode> &nodes);
    int reachable_word_graph_nodes(std::vector<SubwordNode> &nodes);
    void expand_subword_nodes(const std::vector<SubwordNode> &swnodes,
                              std::vector<Node> &nodes,
                              bool debug=false,
                              int sw_node_idx=START_NODE,
                              int node_idx=START_NODE,
                              char left_context='_',
                              char prev_triphone='_',
                              int delayed_subword_id=-1);
    void tie_state_prefixes(std::vector<Node> &nodes,
                            bool debug=false,
                            bool stop_propagation=false,
                            int node_idx=START_NODE);
    void tie_state_suffixes(std::vector<Node> &nodes,
                            bool debug=false,
                            int node_idx=END_NODE);
    void print_graph(std::vector<Node> &nodes);
    int reachable_graph_nodes(std::vector<Node> &nodes);

//private:

    int add_lm_unit(std::string unit);
    void reachable_word_graph_nodes(std::vector<SubwordNode> &nodes,
                                    std::set<int> &node_idxs,
                                    int node_idx=START_NODE);
    void print_word_graph(std::vector<SubwordNode> &nodes,
                          std::vector<int> path,
                          int node_idx=START_NODE);
    int connect_triphone(std::vector<Node> &nodes,
                         std::string triphone,
                         int node_idx,
                         bool debug=false);
    void print_graph(std::vector<Node> &nodes,
                     std::vector<int> path,
                     int node_idx);
    void reachable_graph_nodes(std::vector<Node> &nodes,
                               std::set<int> &node_idxs,
                               int node_idx=START_NODE);
    void prune_unreachable_nodes(std::vector<Node> &nodes);
    void add_hmm_self_transitions(std::vector<Node> &nodes);
    void set_hmm_transition_probs(std::vector<Node> &nodes);
    void push_word_ids_left(std::vector<Node> &nodes,
                            bool debug=false);
    void push_word_ids_left(std::vector<Node> &nodes,
                            std::map<int, std::pair<int, int> > &swaps,
                            bool debug=false,
                            int node_idx=START_NODE,
                            int prev_node_idx=-1,
                            int first_hmm_node_wo_branching=-1);
    void make_swaps(std::vector<Node> &nodes,
                    std::map<int, std::pair<int, int> > &swaps,
                    bool debug=false);
    int num_hmm_states(std::vector<Node> &nodes);
    int num_subword_states(std::vector<Node> &nodes);
    void create_crossword_network(std::vector<Node> &nodes,
                                  std::map<std::string, int> &fanout,
                                  std::map<std::string, int> &fanin,
                                  bool debug=false);
    void set_reverse_arcs(std::vector<Node> &nodes,
                          int node_idx=START_NODE);
    void clear_reverse_arcs(std::vector<Node> &nodes);

    // Text units
    std::vector<std::string> m_units;
    // Mapping from text units to indices
    std::map<std::string, int> m_unit_map;
    // Vocabulary units as phone strings, FIXME: currently only one pronunciation
    std::map<std::string, std::vector<std::string> > m_lexicon;
    // Mapping from phones (triphones) to HMM indices.
    std::map<std::string,int> m_hmm_map;
    // The HMMs.
    std::vector<Hmm> m_hmms;
    // Hmm states
    std::vector<HmmState> m_hmm_states;

    // Word segmentations
    std::map<std::string, std::vector<std::string> > m_word_segs;
};

#endif /* DECODERGRAPH_HH */
