#ifndef SWW_GRAPH_HH
#define SWW_GRAPH_HH

#include <string>
#include <vector>
#include <map>
#include <set>

#include "DecoderGraph.hh"


class SWWGraph : public DecoderGraph {

public:

    SWWGraph();
    SWWGraph(const std::map<std::string, std::vector<std::string> > &word_segs,
             bool wb_symbol=true,
             bool connect_cw_network=true,
             bool verbose=false);

    void create_graph(const std::map<std::string, std::vector<std::string> > &word_segs,
                      bool wb_symbol=true,
                      bool connect_cw_network=true,
                      bool verbose=false);

    void create_crossword_network(const std::map<std::string, std::vector<std::string> > &word_segs,
                                  std::vector<DecoderGraph::Node> &nodes,
                                  std::map<std::string, int> &fanout,
                                  std::map<std::string, int> &fanin,
                                  bool wb_symbol_in_middle=false);

    void tie_graph(bool no_push=false,
                   bool verbose=false);

};

#endif /* SWW_GRAPH_HH */
