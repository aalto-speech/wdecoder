#ifndef WORD_GRAPH_HH
#define WORD_GRAPH_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "DecoderGraph.hh"


class WordGraph : public DecoderGraph {

public:

    WordGraph();
    WordGraph(const std::set<std::string> &words,
              bool verbose=false);

    void create_graph(const std::set<std::string> &words,
                      bool verbose=false);

    void create_crossword_network(const std::set<std::string> &words,
                                  std::vector<DecoderGraph::Node> &nodes,
                                  std::map<std::string, int> &fanout,
                                  std::map<std::string, int> &fanin);

    void connect_one_phone_words_from_start_to_cw(const std::set<std::string> &words,
                                                  std::vector<DecoderGraph::Node> &nodes,
                                                  std::map<std::string, int> &fanout);

    void connect_one_phone_words_from_cw_to_end(const std::set<std::string> &words,
                                                std::vector<DecoderGraph::Node> &nodes,
                                                std::map<std::string, int> &fanin);

    void tie_graph(bool verbose=false,
                   bool remove_cw_markers=false);

};

#endif /* WORD_GRAPH_HH */
