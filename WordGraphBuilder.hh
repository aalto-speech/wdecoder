#ifndef WORD_GRAPH_BUILDER_HH
#define WORD_GRAPH_BUILDER_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "DecoderGraph.hh"


namespace wordgraphbuilder {

void create_crossword_network(DecoderGraph &dg,
                              std::set<std::string> &words,
                              std::vector<DecoderGraph::Node> &nodes,
                              std::map<std::string, int> &fanout,
                              std::map<std::string, int> &fanin);

void connect_one_phone_words_from_start_to_cw(DecoderGraph &dg,
                                              std::set<std::string> &words,
                                              std::vector<DecoderGraph::Node> &nodes,
                                              std::map<std::string, int> &fanout);

void connect_one_phone_words_from_cw_to_end(DecoderGraph &dg,
                                            std::set<std::string> &words,
                                            std::vector<DecoderGraph::Node> &nodes,
                                            std::map<std::string, int> &fanin);

}

#endif /* WORD_GRAPH_BUILDER_HH */
