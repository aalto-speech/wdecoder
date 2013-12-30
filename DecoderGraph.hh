#ifndef DECODERGRAPH_HH
#define DECODERGRAPH_HH

#include <map>
#include <vector>

#include "Hmm.hh"


class DecoderGraph {

public:
    DecoderGraph() { };

    void read_noway_lexicon(std::string lexfname);

private:

    // Mapping from phones (triphones) to HMM indices.
    std::map<std::string,int> m_hmm_map;
    // The HMMs.
    std::vector<Hmm> m_hmms;

};

#endif /* DECODERGRAPH_HH */
