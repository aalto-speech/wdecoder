#ifndef DECODER_HH
#define DECODER_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "Hmm.hh"


class Decoder {

public:
    static const int START_NODE = 0;
    static const int END_NODE = 1;

    class Arc {
    public:
        Arc() : log_prob(0.0), target_node(-1) { }
        float log_prob;
        int target_node;
    };

    class Node {
    public:
        Node() : word_id(-1), hmm_state(-1) { }
        int word_id; // -1 for nodes without word identity.
        int hmm_state; // -1 for nodes without acoustics.
        std::vector<Arc> arcs;
    };

    Decoder() { debug = 0; };

    void read_phone_model(std::string phnfname);
    void read_duration_model(std::string durfname);
    void read_noway_lexicon(std::string lexfname);
    void read_dgraph(std::string graphfname);

    int debug;

    // Subwords
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

    std::vector<Node> m_nodes;
};

#endif /* DECODER_HH */
