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

    void create_crossword_network(std::vector<std::pair<unsigned int, std::string> > &fanout_triphones,
                                  std::vector<std::pair<unsigned int, std::string> > &fanin_triphones,
                                  std::set<std::string> &one_phone_fanout_subwords,
                                  std::set<std::string> &one_phone_fanin_subwords,
                                  std::vector<DecoderGraph::Node> &cw_nodes,
                                  std::map<std::string, int> &fanout,
                                  std::map<std::string, int> &fanin,
                                  bool short_silence=false);

    void connect_crossword_network(std::vector<DecoderGraph::Node> &nodes,
                                   std::vector<std::pair<unsigned int, std::string> > &fanout_connectors,
                                   std::vector<std::pair<unsigned int, std::string> > &fanin_connectors,
                                   std::vector<DecoderGraph::Node> &cw_nodes,
                                   std::map<std::string, int> &fanout,
                                   std::map<std::string, int> &fanin,
                                   bool push_left_after_fanin=true);

    void connect_one_phone_subwords_from_start_to_cw(const std::set<std::string> &subwords,
                                                     std::vector<DecoderGraph::Node> &nodes,
                                                     std::map<std::string, int> &fanout);

    void connect_one_phone_subwords_from_cw_to_end(const std::set<std::string> &subwords,
                                                   std::vector<DecoderGraph::Node> &nodes,
                                                   std::map<std::string, int> &fanin);

    void get_one_phone_subwords(const std::set<std::string> &subwords,
                                std::set<std::string> &one_phone_subwords) const;

    static void offset(std::vector<DecoderGraph::Node> &nodes,
                       int offset);

    static void offset(std::vector<std::pair<unsigned int, std::string> > &connectors,
                       int offset);

    static void collect_crossword_connectors(std::vector<DecoderGraph::Node> &nodes,
                                             std::vector<std::pair<unsigned int, std::string> > &fanout_connectors,
                                             std::vector<std::pair<unsigned int, std::string> > &fanin_connectors);

};

#endif /* NO WB_SUBWORD_GRAPH_HH */
