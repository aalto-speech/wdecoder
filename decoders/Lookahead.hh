#ifndef LOOKAHEAD_HH
#define LOOKAHEAD_HH

#include <vector>
#include <string>

#include "Decoder.hh"
#include "ClassNgram.hh"
#include "DynamicBitset.hh"
#include "QuantizedLogProb.hh"


class UnigramLookahead : public Decoder::Lookahead {
public:
    UnigramLookahead(Decoder &decoder,
                     std::string lafname);
    ~UnigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

private:
    void set_unigram_la_scores();
    void propagate_unigram_la_score(int node_idx,
                                    float score,
                                    std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                    int &la_score_set,
                                    bool start_node=true);
    float set_arc_la_updates();

    std::vector<float> m_la_scores;
};


class DummyBigramLookahead : public Decoder::Lookahead {
public:
    DummyBigramLookahead(Decoder &decoder,
                         std::string lafname);
    ~DummyBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);
};


class BigramLookahead : public Decoder::Lookahead {
protected:
    void set_word_id_la_states();
    void convert_reverse_bigram_idxs(std::map<int, std::vector<int> > &reverse_bigrams);

    std::vector<int> m_word_id_la_state_lookup;
};


class LookaheadStateCount : public BigramLookahead {
public:
    LookaheadStateCount(Decoder &decoder,
                        bool successor_lists=false,
                        bool true_count=false);
    ~LookaheadStateCount() {};
    float get_lookahead_score(int node_idx, int word_id) { throw std::string("Not a real lookahead."); }
    int la_state_count() { return m_la_state_count; }
protected:
    int set_la_state_indices_to_nodes();
    void propagate_la_state_idx(int node_idx,
                                std::map<int, int> &la_state_changes,
                                long long int &max_state_idx,
                                std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                bool first_node=true);
    int set_la_state_successor_lists();
    float set_arc_la_updates();

    int m_la_state_count;
    std::vector<int> m_node_la_states;
    std::vector<std::vector<int> > m_la_state_successor_words;
};


class FullTableBigramLookahead : public LookaheadStateCount {
public:
    FullTableBigramLookahead(Decoder &decoder,
                             std::string lafname,
                             bool successor_lists=true,
                             bool quantization=false);
    ~FullTableBigramLookahead() {};
    virtual float get_lookahead_score(int node_idx, int word_id);

protected:
    std::vector<std::vector<float> > m_bigram_la_scores;
};


class PrecomputedFullTableBigramLookahead : public FullTableBigramLookahead {
public:
    PrecomputedFullTableBigramLookahead(Decoder &decoder,
                                        std::string lafname,
                                        bool quantization=false);
    ~PrecomputedFullTableBigramLookahead() {};
    virtual float get_lookahead_score(int node_idx, int word_id);

private:
    void find_preceeding_la_states(int node_idx,
                                   std::set<int> &la_states,
                                   const std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                   bool first_node=true,
                                   bool state_change=true);

    void set_unigram_la_scores();
    void propagate_unigram_la_score(int node_idx,
                                    float score,
                                    int word_id,
                                    std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                    std::vector<std::pair<int, float> > &unigram_la_scores,
                                    bool start_node);
    void set_bigram_la_scores();
    void set_lookahead_score(int la_state_idx, int word_id, float la_score);

    bool m_quantization;
    QuantizedLogProb m_quant_log_probs;
    std::vector<std::vector<unsigned short int> > m_quant_bigram_lookup;
    double m_min_la_score;
};


// Full bigram tables for cross-word and initial nodes
// Vectors with binary search in inner nodes
class HybridBigramLookahead : public BigramLookahead {
public:
    HybridBigramLookahead(Decoder &decoder,
                          std::string lafname);
    ~HybridBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

protected:
    int set_la_state_indices_to_nodes();
    void propagate_la_state_idx(int node_idx,
                                int la_state_idx,
                                int &max_state_idx,
                                bool first_node=true);
    int set_la_state_successor_lists();
    int set_bigram_la_maps();
    float set_arc_la_updates();

    std::vector<int> m_node_la_states;
    std::vector<std::vector<int> > m_la_state_successor_words;
    std::vector<std::vector<float> > m_bigram_la_scores;
    std::vector<std::map<int, float> > m_bigram_la_maps;
    std::vector<std::vector<int> > m_inner_bigram_score_idxs;
    std::vector<std::vector<float> > m_inner_bigram_scores;
    std::vector<bool> m_single_inner_bigram_score;
};


// Precomputed table values
class PrecomputedHybridBigramLookahead : public HybridBigramLookahead {
public:
    PrecomputedHybridBigramLookahead(Decoder &decoder,
                                     std::string lafname);
    ~PrecomputedHybridBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

private:
    void find_preceeding_la_states(int node_idx,
                                   std::set<int> &la_states,
                                   const std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                   bool first_node=true,
                                   bool state_change=true);
    void find_first_lm_nodes(std::set<int> &lm_nodes,
                             int node_idx=START_NODE);
    void find_cw_lm_nodes(std::set<int> &lm_nodes);
    void find_sentence_end_lm_node(std::set<int> &lm_nodes);

    void set_unigram_la_scores();
    void propagate_unigram_la_score(int node_idx,
                                    float score,
                                    int word_id,
                                    std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                    std::vector<std::pair<int, float> > &unigram_la_scores,
                                    bool start_node);
    void set_bigram_la_scores();
};


class LargeBigramLookahead : public BigramLookahead {
public:
    LargeBigramLookahead(Decoder &decoder,
                         std::string lafname);
    LargeBigramLookahead(Decoder &decoder,
                         std::string lafname,
                         std::string statesfname);
    ~LargeBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);
    void write(std::string ofname);
    void read(std::string ifname);

private:
    int set_la_state_indices_to_nodes();
    float set_arc_la_updates();
    void propagate_la_state_idx(int node_idx,
                                int la_state_idx,
                                int &max_state_idx,
                                bool first_node=true);
    void find_preceeding_la_states(int node_idx,
                                   std::set<int> &la_states,
                                   const std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                   bool first_node=true,
                                   bool state_change=true);

    void set_unigram_la_scores();
    void propagate_unigram_la_score(int node_idx,
                                    float score,
                                    int word_id,
                                    std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                    bool start_node);

    int set_bigram_la_scores();
    void propagate_bigram_la_scores(int node_idx,
                                    int word_id,
                                    std::vector<int> predecessor_words,
                                    std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                    std::set<int> &processed_la_states,
                                    bool start_node,
                                    bool la_state_change);
    int set_bigram_la_scores_2();

    class LookaheadState {
    public:
        LookaheadState() : m_best_unigram_word_id(-1),
                           m_best_unigram_score(-1e20) { }
        std::map<int, float> m_scores;
        int m_best_unigram_word_id;
        float m_best_unigram_score;
    };

    std::vector<int> m_node_la_states;
    std::vector<LookaheadState> m_lookahead_states;
};

#endif /* LOOKAHEAD_HH */
