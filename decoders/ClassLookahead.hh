#ifndef CLASS_LOOKAHEAD_HH
#define CLASS_LOOKAHEAD_HH

#include <vector>
#include <string>

#include "Decoder.hh"
#include "Lookahead.hh"
#include "ClassNgram.hh"
#include "DynamicBitset.hh"
#include "QuantizedLogProb.hh"

class DummyClassBigramLookahead : public Decoder::Lookahead {
public:
    DummyClassBigramLookahead(Decoder &decoder,
                              std::string carpafname,
                              std::string classmfname);
    ~DummyClassBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

    ClassNgram m_class_la;
};

class ClassBigramLookahead : public Decoder::Lookahead {
public:
    ClassBigramLookahead(Decoder &decoder,
                         std::string carpafname,
                         std::string classmfname,
                         bool quantization=false,
                         std::string nodeStates="");
    ~ClassBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);
    void readStates(std::string ifname);
    void writeStates(std::string ofname) const;

    ClassNgram m_class_la;
private:
    int set_la_state_indices_to_nodes();
    float set_arc_la_updates();
    void init_la_scores();
    void set_la_score(int la_state, int class_idx, float la_prob);
    void set_la_scores();
    std::vector<int> m_node_la_states;
    int m_la_state_count;

    bool m_quantization;

    // Normal look-ahead scores
    std::vector<std::vector<float> > m_la_scores;

    // Quantized look-ahead scores
    QuantizedLogProb m_quant_log_probs;
    std::vector<std::vector<unsigned short int> > m_quant_bigram_lookup;
    double m_min_la_score;
};

#endif /* LOOKAHEAD_HH */
