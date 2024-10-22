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

    void read_lexicon(std::string lexfname,
                      std::set<std::string> &prefix_subwords,
                      std::set<std::string> &stem_subwords,
                      std::set<std::string> &suffix_subwords,
                      std::set<std::string> &word_subwords);

    void create_crossunit_network(
        std::vector<std::pair<unsigned int, std::string> > &fanout_triphones,
        std::vector<std::pair<unsigned int, std::string> > &fanin_triphones,
        std::vector<std::pair<unsigned int, std::string> > &cw_fanout_triphones,
        std::set<std::string> &one_phone_prefix_subwords,
        std::set<std::string> &one_phone_stem_subwords,
        std::set<std::string> &one_phone_suffix_subwords,
        std::vector<DecoderGraph::Node> &cw_nodes,
        std::map<std::string, int> &fanout,
        std::map<std::string, int> &fanin);

    void create_crossword_network(
        std::vector<std::pair<unsigned int, std::string> > &fanout_triphones,
        std::vector<std::pair<unsigned int, std::string> > &fanin_triphones,
        std::vector<std::pair<unsigned int, std::string> > &cu_fanout_triphones,
        std::set<std::string> &one_phone_suffix_subwords,
        std::vector<DecoderGraph::Node> &cw_nodes,
        std::map<std::string, int> &fanout,
        std::map<std::string, int> &fanin);

    void connect_crossword_network(std::vector<DecoderGraph::Node> &nodes,
                                   std::vector<std::pair<unsigned int, std::string> > &fanout_connectors,
                                   std::vector<std::pair<unsigned int, std::string> > &fanin_connectors,
                                   std::vector<DecoderGraph::Node> &cw_nodes,
                                   std::map<std::string, int> &fanout,
                                   std::map<std::string, int> &fanin);

    void connect_one_phone_subwords_from_start_to_cw(const std::set<std::string> &subwords,
            std::vector<DecoderGraph::Node> &nodes,
            std::map<std::string, int> &fanout);

    void connect_one_phone_subwords_from_cw_to_end(const std::set<std::string> &subwords,
            std::vector<DecoderGraph::Node> &nodes,
            std::map<std::string, int> &fanin);

    void connect_one_phone_prefix_subwords_to_cu_fanout(
        const std::set<std::string> &one_phone_prefix_subwords,
        std::vector<DecoderGraph::Node> &nodes,
        std::vector<std::pair<unsigned int, std::string> > &fanout_connectors,
        std::map<std::string, int> &fanout);

    void connect_one_phone_suffix_subwords_to_cw_fanout(
        const std::set<std::string> &one_phone_suffix_subwords,
        std::vector<DecoderGraph::Node> &nodes,
        std::vector<std::pair<unsigned int, std::string> > &fanout_connectors,
        std::map<std::string, int> &fanout);

    void connect_one_phone_suffix_subwords_from_cu_to_cw_fanout(
        const std::set<std::string> &one_phone_suffix_subwords,
        std::vector<DecoderGraph::Node> &nodes,
        std::map<std::string, int> &cu_fanout,
        std::map<std::string, int> &cw_fanout);

    void get_one_phone_prefix_subwords(const std::set<std::string> &prefix_subwords,
                                       std::set<std::string> &one_phone_prefix_subwords);

    void get_one_phone_stem_subwords(const std::set<std::string> &stem_subwords,
                                     std::set<std::string> &one_phone_stem_subwords);

    void get_one_phone_suffix_subwords(const std::set<std::string> &suffix_subwords,
                                       std::set<std::string> &one_phone_suffix_subwords);

    static void offset(std::vector<DecoderGraph::Node> &nodes,
                       int offset);

    static void offset(std::vector<std::pair<unsigned int, std::string> > &connectors,
                       int offset);

    void create_graph(const std::set<std::string> &prefix_subwords,
                      const std::set<std::string> &stem_subwords,
                      const std::set<std::string> &suffix_subwords,
                      const std::set<std::string> &word_subwords,
                      bool verbose=false);
};

#endif /* LRWB_SUBWORD_GRAPH_HH */
