#ifndef SUBWORD_GRAPH_HH
#define SUBWORD_GRAPH_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "DecoderGraph.hh"


class SubwordGraph : public DecoderGraph {

public:

    SubwordGraph();
    SubwordGraph(const std::set<std::string> &subwords,
                 bool verbose=false);
    virtual ~SubwordGraph() { };

    void create_graph(const std::set<std::string> &subwords,
                      bool verbose=false);

    void create_crossword_network(const std::set<std::string> &subwords,
                                  std::vector<DecoderGraph::Node> &nodes,
                                  std::map<std::string, int> &fanout,
                                  std::map<std::string, int> &fanin);

    void connect_one_phone_subwords_from_start_to_cw(const std::set<std::string> &subwords,
                                                     std::vector<DecoderGraph::Node> &nodes,
                                                     std::map<std::string, int> &fanout);

    void connect_one_phone_subwords_from_cw_to_end(const std::set<std::string> &subwords,
                                                   std::vector<DecoderGraph::Node> &nodes,
                                                   std::map<std::string, int> &fanin);

    virtual void create_forced_path(std::vector<DecoderGraph::Node> &nodes,
                                    std::vector<std::string> &sentence,
                                    std::map<int, std::string> &node_labels);

    void add_long_silence_no_start_end_wb();
};

#endif /* SUBWORD_GRAPH_HH */
