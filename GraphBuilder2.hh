#ifndef GRAPH_BUILDER_2_HH
#define GRAPH_BUILDER_2_HH

#include <string>
#include <vector>
#include <map>
#include <set>

#include "DecoderGraph.hh"


namespace graphbuilder2 {

void create_crossword_network(DecoderGraph &dg,
                              std::map<std::string, std::vector<std::string> > &word_segs,
                              std::vector<DecoderGraph::Node> &nodes,
                              std::map<std::string, int> &fanout,
                              std::map<std::string, int> &fanin);

void connect_crossword_network(DecoderGraph &dg,
                               std::vector<DecoderGraph::Node> &nodes,
                               std::vector<DecoderGraph::Node> &cw_nodes,
                               std::map<std::string, int> &fanout,
                               std::map<std::string, int> &fanin,
                               bool push_left_after_fanin=true);

}

#endif /* GRAPH_BUILDER_2_HH */
