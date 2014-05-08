#ifndef GRAPH_BUILDER_1_HH
#define GRAPH_BUILDER_1_HH

#include <string>
#include <vector>
#include <map>
#include <set>

#include "DecoderGraph.hh"


namespace graphbuilder1 {

class SubwordNode {
public:
    SubwordNode() { connected_from_start_node = false; }
    std::vector<int> subword_ids;
    //std::vector<std::pair<std::vector<int>, unsigned int> > in_arcs;  // subword ids (lookahead), node id
    std::multimap<std::vector<int>, unsigned int> in_arcs;              // subword ids (lookahead), node id
    std::map<std::vector<int>, unsigned int> out_arcs;                  // subword ids (lookahead), node id
    std::vector<std::string> triphones;
    bool connected_from_start_node;
};

void create_word_graph(DecoderGraph &dg,
                       std::vector<SubwordNode> &nodes,
                       std::map<std::string, std::vector<std::string> > &word_segs);
void tie_subword_suffixes(std::vector<SubwordNode> &nodes);
void print_word_graph(DecoderGraph &dg,
                      std::vector<SubwordNode> &nodes);
void print_word_graph(DecoderGraph &dg,
                      std::vector<SubwordNode> &nodes,
                      std::vector<int> path,
                      int node_idx=START_NODE);
int reachable_word_graph_nodes(std::vector<SubwordNode> &nodes);
void reachable_word_graph_nodes(std::vector<SubwordNode> &nodes,
                                std::set<int> &node_idxs,
                                int node_idx=START_NODE);
void triphonize_subword_nodes(DecoderGraph &dg,
                              std::vector<SubwordNode> &swnodes);
void expand_subword_nodes(DecoderGraph &dg,
                          std::vector<SubwordNode> &swnodes,
                          std::vector<DecoderGraph::Node> &nodes);
void expand_subword_nodes(DecoderGraph &dg,
                          const std::vector<SubwordNode> &swnodes,
                          std::vector<DecoderGraph::Node> &nodes,
                          std::map<sw_node_idx_t, node_idx_t> &expanded_nodes,
                          sw_node_idx_t sw_node_idx=START_NODE,
                          node_idx_t node_idx=START_NODE,
                          char left_context='_',
                          char prev_triphone='_');
void create_crossword_network(DecoderGraph &dg,
                              std::map<std::string, std::vector<std::string> > &word_segs,
                              std::vector<DecoderGraph::Node> &nodes,
                              std::map<std::string, int> &fanout,
                              std::map<std::string, int> &fanin,
                              bool wb_symbol=false);
void connect_crossword_network(DecoderGraph &dg,
                               std::vector<DecoderGraph::Node> &nodes,
                               std::vector<DecoderGraph::Node> &cw_nodes,
                               std::map<std::string, int> &fanout,
                               std::map<std::string, int> &fanin,
                               bool push_left_after_fanin=true);

}

#endif /* GRAPH_BUILDER_1_HH */
