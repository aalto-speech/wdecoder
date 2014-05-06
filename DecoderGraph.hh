#ifndef DECODERGRAPH_HH
#define DECODERGRAPH_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"


class DecoderGraph {

public:

    class TriphoneNode {
    public:
        TriphoneNode() : subword_id(-1), hmm_id(-1), connect_to_end_node(false) { }
        int subword_id; // -1 for nodes without word identity.
        int hmm_id; // -1 for nodes without acoustics.
        bool connect_to_end_node;
        std::map<int, unsigned int> hmm_id_lookahead;
        std::map<int, unsigned int> subword_id_lookahead;
    };

    class Node {
    public:
        Node() : word_id(-1), hmm_state(-1), flags(0) { }
        int word_id; // -1 for nodes without word identity.
        int hmm_state; // -1 for nodes without acoustics.
        int flags;
        std::set<unsigned int> arcs;
        std::set<unsigned int> reverse_arcs;
    };

    int debug;

    // Text units
    std::vector<std::string> m_subwords;
    // Mapping from text units to indices
    std::map<std::string, int> m_subword_map;
    // Vocabulary units as phone strings, FIXME: currently only one pronunciation
    std::map<std::string, std::vector<std::string> > m_lexicon;
    // Mapping from phones (triphones) to HMM indices.
    std::map<std::string,int> m_hmm_map;
    // The HMMs.
    std::vector<Hmm> m_hmms;
    // Hmm states
    std::vector<HmmState> m_hmm_states;

    // Word segmentations
    std::map<std::string, std::vector<std::string> > m_word_segs;

    DecoderGraph() { debug = 0; };

    void read_phone_model(std::string phnfname);
    void read_noway_lexicon(std::string lexfname);

    void collect_cw_fanout_nodes(std::vector<Node> &nodes,
                                 std::map<int, std::string> &nodes_to_fanout,
                                 int hmm_state_count=0,
                                 std::vector<char> phones = std::vector<char>(),
                                 int node_to_connect=-1,
                                 int node_idx=END_NODE);
    void collect_cw_fanin_nodes(std::vector<Node> &nodes,
                                std::map<node_idx_t, std::string> &nodes_from_fanin,
                                int hmm_state_count=0,
                                std::vector<char> phones = std::vector<char>(),
                                node_idx_t node_to_connect=START_NODE,
                                node_idx_t node_idx=START_NODE);


};

#endif /* DECODERGRAPH_HH */
