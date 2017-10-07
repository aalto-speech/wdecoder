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
};

#endif /* LRWB_SUBWORD_GRAPH_HH */
