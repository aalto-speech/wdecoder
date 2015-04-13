#ifndef SUBWORD_GRAPH_BUILDER_HH
#define SUBWORD_GRAPH_BUILDER_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "DecoderGraph.hh"


namespace subwordgraphbuilder {

void create_crossword_network(DecoderGraph &dg,
                              std::set<std::string> &subwords,
                              std::vector<DecoderGraph::Node> &nodes,
                              std::map<std::string, int> &fanout,
                              std::map<std::string, int> &fanin);

void connect_crossword_network(DecoderGraph &dg,
                               std::vector<DecoderGraph::Node> &nodes,
                               std::vector<DecoderGraph::Node> &cw_nodes,
                               std::map<std::string, int> &fanout,
                               std::map<std::string, int> &fanin,
                               bool push_left_after_fanin=true);

void connect_one_phone_subwords_from_start_to_cw(DecoderGraph &dg,
                                                 std::set<std::string> &subwords,
                                                 std::vector<DecoderGraph::Node> &nodes,
                                                 std::map<std::string, int> &fanout);

void connect_one_phone_subwords_from_cw_to_end(DecoderGraph &dg,
                                               std::set<std::string> &subwords,
                                               std::vector<DecoderGraph::Node> &nodes,
                                               std::map<std::string, int> &fanin);

void create_forced_path(DecoderGraph &dg,
                        std::vector<DecoderGraph::Node> &nodes,
                        std::vector<std::string> &sentence,
                        std::map<int, std::string> &node_labels);

void create_graph(DecoderGraph &dg,
                  std::vector<DecoderGraph::Node> &nodes,
                  const std::map<std::string, std::vector<std::string> > &word_segs);

}

#endif /* SUBWORD_GRAPH_BUILDER_HH */
