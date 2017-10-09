#ifndef LRWB_SUBWORD_GRAPH_HH
#define LRWB_SUBWORD_GRAPH_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"
#include "DecoderGraph.hh"

class LRWBSubwordGraph : public DecoderGraph {
public:
    LRWBSubwordGraph();
    LRWBSubwordGraph(const std::set<std::string> &prefix_subwords,
                     const std::set<std::string> &stem_subwords,
                     const std::set<std::string> &suffix_subwords,
                     const std::set<std::string> &word_subwords,
                     bool verbose=false);

    void create_graph(const std::set<std::string> &prefix_subwords,
                      const std::set<std::string> &stem_subwords,
                      const std::set<std::string> &suffix_subwords,
                      const std::set<std::string> &word_subwords,
                      bool verbose=false);

    void read_lexicon(std::string lexfname,
                      std::set<std::string> &prefix_subwords,
                      std::set<std::string> &stem_subwords,
                      std::set<std::string> &suffix_subwords,
                      std::set<std::string> &word_subwords);

    void create_crossword_network(const std::set<std::string> &word_subwords,
                                  std::vector<DecoderGraph::Node> &nodes,
                                  std::map<std::string, int> &fanout,
                                  std::map<std::string, int> &fanin);

    void connect_one_phone_words_from_start_to_cw(const std::set<std::string> &word_subwords,
                                                  std::vector<DecoderGraph::Node> &nodes,
                                                  std::map<std::string, int> &fanout);

    void connect_one_phone_words_from_cw_to_end(const std::set<std::string> &word_subwords,
                                                std::vector<DecoderGraph::Node> &nodes,
                                                std::map<std::string, int> &fanin);

};

#endif /* LRWB_SUBWORD_GRAPH_HH */
