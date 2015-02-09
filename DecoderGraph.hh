#ifndef DECODERGRAPH_HH
#define DECODERGRAPH_HH

#include <map>
#include <fstream>
#include <vector>
#include <set>

#include "defs.hh"
#include "Hmm.hh"


class TriphoneNode {
public:
    TriphoneNode() : subword_id(-1), hmm_id(-1) { }
    TriphoneNode(int subword_id, int hmm_id) : subword_id(subword_id), hmm_id(hmm_id) { }
    int subword_id; // -1 for nodes without word identity.
    int hmm_id; // -1 for nodes without acoustics.
};


class DecoderGraph {

public:

    class Node {
    public:
        Node() : word_id(-1), hmm_state(-1), flags(0), lookahead(nullptr) { }
        int word_id; // -1 for nodes without word identity.
        int hmm_state; // -1 for nodes without acoustics.
        int flags;
        std::set<unsigned int> arcs;
        std::set<unsigned int> reverse_arcs;

        // word_id, hmm_state pairs
        // used in construction
        std::map<std::pair<int, int>, int> *lookahead;

        int find_next(int word_id, int hmm_state) {
            if (lookahead == nullptr) throw std::string("Lookahead not set.");
            std::pair<int,int> id = std::make_pair(word_id, hmm_state);
            if (lookahead->find(id) != lookahead->end())
                return (*lookahead)[id];
            else return -1;
        }
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
    //std::map<std::string, std::vector<std::string> > m_word_segs;

    DecoderGraph() {
        debug = 0;
    };

    void read_phone_model(std::string phnfname);
    void read_noway_lexicon(std::string lexfname);

};

#endif /* DECODERGRAPH_HH */
