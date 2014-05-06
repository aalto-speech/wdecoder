#ifndef GRAPH_BUILDER_2_HH
#define GRAPH_BUILDER_2_HH

#include "DecoderGraph.hh"


namespace graphbuilder2 {

static void read_word_segmentations(DecoderGraph &dg,
                                    std::string segfname,
                                    std::vector<std::pair<std::string, std::vector<std::string> > > &word_segs);

static void create_crossword_network(std::vector<std::pair<std::string, std::vector<std::string> > > &word_segs,
                              std::vector<DecoderGraph::Node> &nodes,
                              std::map<std::string, int> &fanout,
                              std::map<std::string, int> &fanin);

static void connect_crossword_network(std::vector<DecoderGraph::Node> &nodes,
                               std::vector<DecoderGraph::Node> &cw_nodes,
                               std::map<std::string, int> &fanout,
                               std::map<std::string, int> &fanin,
                               bool push_left_after_fanin=true);

}

#endif /* GRAPH_BUILDER_2_HH */
