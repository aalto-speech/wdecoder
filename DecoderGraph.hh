#ifndef DECODERGRAPH_HH
#define DECODERGRAPH_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"


class DecoderGraph {

public:

    struct SubwordNode {
        std::vector<int> subword_ids;
        std::vector<std::pair<std::vector<int>, int> > in_arcs;  // subword ids (lookahead), node id
        std::map<std::vector<int>, int> out_arcs;                // subword ids (lookahead), node id
        std::vector<std::string> triphones;
    };

    class Node {
    public:
        Node() : word_id(-1), hmm_state(-1), flags(0) { }
        int word_id; // -1 for nodes without word identity.
        int hmm_state; // -1 for nodes without acoustics.
        int flags;
        std::set<int> arcs;
        std::set<int> reverse_arcs;
    };

    DecoderGraph() { debug = 0; };

    void read_phone_model(std::string phnfname);
    void read_noway_lexicon(std::string lexfname);
    void read_word_segmentations(std::string segfname);

    void create_word_graph(std::vector<SubwordNode> &nodes);
    void tie_subword_suffixes(std::vector<SubwordNode> &nodes);
    void print_word_graph(std::vector<SubwordNode> &nodes);
    int reachable_word_graph_nodes(std::vector<SubwordNode> &nodes);
    void triphonize_subword_nodes(std::vector<SubwordNode> &swnodes);
    void expand_subword_nodes(std::vector<SubwordNode> &swnodes,
                              std::vector<Node> &nodes);
    void expand_subword_nodes(const std::vector<SubwordNode> &swnodes,
                              std::vector<Node> &nodes,
                              std::map<int, int> &expanded_nodes,
                              int sw_node_idx=START_NODE,
                              int node_idx=START_NODE,
                              char left_context='_',
                              char prev_triphone='_');
    void tie_state_prefixes(std::vector<Node> &nodes,
                            bool stop_propagation=false);
    void tie_state_prefixes(std::vector<Node> &nodes,
                            std::set<int> &processed_nodes,
                            bool stop_propagation=false,
                            int node_idx=START_NODE);
    void tie_state_suffixes(std::vector<Node> &nodes,
                            bool stop_propagation=false);
    void tie_state_suffixes(std::vector<Node> &nodes,
                            std::set<int> &processed_nodes,
                            bool stop_propagation=false,
                            int node_idx=END_NODE);
    void tie_word_id_prefixes(std::vector<Node> &nodes,
                              bool stop_propagation=false);
    void tie_word_id_prefixes(std::vector<Node> &nodes,
                              std::set<int> &processed_nodes,
                              bool stop_propagation=false,
                              int node_idx=START_NODE);
    void tie_word_id_suffixes(std::vector<Node> &nodes,
                              bool stop_propagation=false);
    void tie_word_id_suffixes(std::vector<Node> &nodes,
                              std::set<int> &processed_nodes,
                              bool stop_propagation=false,
                              int node_idx=END_NODE);
    void print_graph(std::vector<Node> &nodes);
    void print_dot_digraph(std::vector<Node> &nodes, std::ostream &fstr = std::cout);
    int reachable_graph_nodes(std::vector<Node> &nodes);

    int debug;

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
                         int node_idx);
    void print_graph(std::vector<Node> &nodes,
                     std::vector<int> path,
                     int node_idx);
    void reachable_graph_nodes(std::vector<Node> &nodes,
                               std::set<int> &node_idxs,
                               int node_idx=START_NODE);
    void prune_unreachable_nodes(std::vector<Node> &nodes);
    void add_hmm_self_transitions(std::vector<Node> &nodes);
    void push_word_ids_left(std::vector<Node> &nodes);
    void push_word_ids_left(std::vector<Node> &nodes,
                            int &move_count,
                            std::set<int> &processed_nodes,
                            int node_idx=END_NODE,
                            int prev_node_idx=-1,
                            int subword_id=-1);
    void push_word_ids_right(std::vector<Node> &nodes);
    void push_word_ids_right(std::vector<Node> &nodes,
                             int &move_count,
                             std::set<int> &processed_nodes,
                             int node_idx=START_NODE,
                             int prev_node_idx=-1,
                             int subword_id=-1);
    int num_hmm_states(std::vector<Node> &nodes);
    int num_subword_states(std::vector<Node> &nodes);
    void create_crossword_network(std::vector<Node> &nodes,
                                  std::map<std::string, int> &fanout,
                                  std::map<std::string, int> &fanin);
    void connect_crossword_network(std::vector<Node> &nodes,
                                   std::vector<Node> &cw_nodes,
                                   std::map<std::string, int> &fanout,
                                   std::map<std::string, int> &fanin);
    void collect_cw_fanout_nodes(std::vector<Node> &nodes,
                                 std::map<int, std::string> &nodes_to_fanout,
                                 int hmm_state_count=0,
                                 std::vector<char> phones = std::vector<char>(),
                                 int node_to_connect=-1,
                                 int node_idx=END_NODE);
    void collect_cw_fanin_nodes(std::vector<Node> &nodes,
                                std::map<int, std::string> &nodes_from_fanin,
                                int hmm_state_count=0,
                                std::vector<char> phones = std::vector<char>(),
                                int node_to_connect=-1,
                                int node_idx=START_NODE);
    void set_reverse_arcs_also_from_unreachable(std::vector<Node> &nodes);
    void set_reverse_arcs(std::vector<Node> &nodes);
    void set_reverse_arcs(std::vector<Node> &nodes,
                          int node_idx,
                          std::set<int> &processed_nodes);
    void clear_reverse_arcs(std::vector<Node> &nodes);
    int merge_nodes(std::vector<Node> &nodes, int node_idx_1, int node_idx_2);
    void connect_end_to_start_node(std::vector<Node> &nodes);
    void write_graph(std::vector<Node> &nodes, std::string fname);

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
