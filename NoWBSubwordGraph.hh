#ifndef NO_WB_SUBWORD_GRAPH_HH
#define NO_WB_SUBWORD_GRAPH_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "DecoderGraph.hh"


class NoWBSubwordGraph : public DecoderGraph {

public:

    NoWBSubwordGraph();
    NoWBSubwordGraph(const std::set<std::string> &word_start_subwords,
                     const std::set<std::string> &subwords,
                     bool verbose=false);

    void create_graph(const std::set<std::string> &word_start_subwords,
                      const std::set<std::string> &subwords,
                      bool verbose=false);

    void create_crossword_network(const std::set<std::string> &fanout_subwords,
                                  const std::set<std::string> &fanin_subwords,
                                  std::vector<DecoderGraph::Node> &nodes,
                                  std::map<std::string, int> &fanout,
                                  std::map<std::string, int> &fanin);

    void connect_one_phone_subwords_from_start_to_cw(const std::set<std::string> &subwords,
                                                     std::vector<DecoderGraph::Node> &nodes,
                                                     std::map<std::string, int> &fanout);

    void connect_one_phone_subwords_from_cw_to_end(const std::set<std::string> &subwords,
                                                   std::vector<DecoderGraph::Node> &nodes,
                                                   std::map<std::string, int> &fanin);

    void offset(std::vector<DecoderGraph::Node> &nodes,
                int offset);

};

#endif /* NO WB_SUBWORD_GRAPH_HH */
