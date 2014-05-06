#ifndef GRAPH_BUILDER_1_HH
#define GRAPH_BUILDER_1_HH

#include "DecoderGraph.hh"


namespace graphbuilder1 {

    struct SubwordNode {
        std::vector<int> subword_ids;
        std::vector<std::pair<std::vector<int>, unsigned int> > in_arcs;  // subword ids (lookahead), node id
        std::map<std::vector<int>, unsigned int> out_arcs;                // subword ids (lookahead), node id
        std::vector<std::string> triphones;
    };

    static void read_word_segmentations(DecoderGraph &dg,
                                        std::string segfname,
                                        std::vector<std::pair<std::string, std::vector<std::string> > > &word_segs);

    static void create_word_graph(DecoderGraph &dg,
                        std::vector<SubwordNode> &nodes,
            std::vector<std::pair<std::string, std::vector<std::string> > > &word_segs);
    static void tie_subword_suffixes(std::vector<SubwordNode> &nodes);
    static void print_word_graph(DecoderGraph &dg,
                        std::vector<SubwordNode> &nodes);
    static void print_word_graph(DecoderGraph &dg,
                        std::vector<SubwordNode> &nodes,
                          std::vector<int> path,
                          int node_idx=START_NODE);
    static int reachable_word_graph_nodes(std::vector<SubwordNode> &nodes);
    static void reachable_word_graph_nodes(std::vector<SubwordNode> &nodes,
                                    std::set<int> &node_idxs,
                                    int node_idx=START_NODE);
    static void triphonize_subword_nodes(std::vector<SubwordNode> &swnodes);
    static void expand_subword_nodes(std::vector<SubwordNode> &swnodes,
                              std::vector<DecoderGraph::Node> &nodes);
    static void expand_subword_nodes(const std::vector<SubwordNode> &swnodes,
                              std::vector<DecoderGraph::Node> &nodes,
                              std::map<sw_node_idx_t, node_idx_t> &expanded_nodes,
                              sw_node_idx_t sw_node_idx=START_NODE,
                              node_idx_t node_idx=START_NODE,
                              char left_context='_',
                              char prev_triphone='_');

    static void create_crossword_network(DecoderGraph &dg,
            std::vector<std::pair<std::string, std::vector<std::string> > > &word_segs,
            std::vector<DecoderGraph::Node> &nodes,
                                  std::map<std::string, int> &fanout,
                                  std::map<std::string, int> &fanin);
    static void connect_crossword_network(std::vector<DecoderGraph::Node> &nodes,
                                   std::vector<DecoderGraph::Node> &cw_nodes,
                                   std::map<std::string, int> &fanout,
                                   std::map<std::string, int> &fanin,
                                   bool push_left_after_fanin=true);

}

#endif /* GRAPH_BUILDER_1_HH */
