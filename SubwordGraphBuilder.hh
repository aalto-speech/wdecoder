#ifndef DecoderGraphBUILDER_HH
#define DecoderGraphBUILDER_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "DecoderGraph.hh"


namespace SubwordGraphBuilder {

    static void create_crossword_network(std::vector<DecoderGraph::Node> &nodes,
                                         std::map<std::string, int> &fanout,
                                         std::map<std::string, int> &fanin);
    static void connect_crossword_network(std::vector<DecoderGraph::Node> &nodes,
                                          std::vector<DecoderGraph::Node> &cw_nodes,
                                          std::map<std::string, int> &fanout,
                                          std::map<std::string, int> &fanin,
                                          bool push_left_after_fanin=true);
    static void connect_one_phone_subwords_from_start_to_cw(std::vector<DecoderGraph::Node> &nodes,
                                                            std::map<std::string, int> &fanout);
    static void connect_one_phone_subwords_from_cw_to_end(std::vector<DecoderGraph::Node> &nodes,
                                                          std::map<std::string, int> &fanin);

};

#endif /* DecoderGraph_HH */
