#ifndef DECODERGRAPH_HH
#define DECODERGRAPH_HH

#include <map>
#include <vector>

#include "Hmm.hh"


class DecoderGraph {

public:
    DecoderGraph() { };

    void read_phone_model(std::string phnfname);
    void read_duration_model(std::string durfname);
    void read_noway_lexicon(std::string lexfname);
    void read_word_segmentations(std::string segfname);

private:

    int add_lm_unit(std::string unit);

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

    std::map<std::string, std::vector<std::string> >m_word_segs;
};

#endif /* DECODERGRAPH_HH */
