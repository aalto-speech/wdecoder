#ifndef DECODERGRAPH_HH
#define DECODERGRAPH_HH

#include <map>
#include <vector>
#include <set>

#include "Hmm.hh"


class DecoderGraph {

public:
    struct SubwordNode {
        int subword_id;
        std::vector<std::pair<int, int> > in_arcs;  // subword id (lookahead), node id
        std::map<int, int> out_arcs;                // subword id (lookahead), node id
    };
    static const int START_NODE = 0;
    static const int END_NODE = 1;

    DecoderGraph() : m_num_models(0) { };

    void read_phone_model(std::string phnfname);
    void read_duration_model(std::string durfname);
    void read_noway_lexicon(std::string lexfname);
    void read_word_segmentations(std::string segfname);

    void create_word_graph(std::vector<SubwordNode> &nodes);
    void tie_word_graph_suffixes(std::vector<SubwordNode> &nodes);
    void print_word_graph(std::vector<SubwordNode> &nodes);
    int reachable_word_graph_nodes(std::vector<SubwordNode> &nodes);

private:

    int add_lm_unit(std::string unit);
    void reachable_word_graph_nodes(std::vector<SubwordNode> &nodes,
                                    std::set<int> &node_idxs,
                                    int node_idx=START_NODE);
    void print_word_graph(std::vector<SubwordNode> &nodes,
                          std::vector<int> path,
                          int node_idx=START_NODE);

    // Text units
    std::vector<std::string> m_units;
    // Mapping from text units to indices
    std::map<std::string, int> m_unit_map;

    // Mapping from phones (triphones) to HMM indices.
    std::map<std::string,int> m_hmm_map;
    // The HMMs.
    std::vector<Hmm> m_hmms;
    // Number of (tied) states
    int m_num_models;

    std::map<std::string, std::vector<std::string> > m_word_segs;
};

#endif /* DECODERGRAPH_HH */
