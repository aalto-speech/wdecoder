#ifndef LOOKAHEAD_HH
#define LOOKAHEAD_HH

#include <vector>

#include "SimpleHashCache.hh"
#include "Decoder.hh"


class NoLookahead : public Decoder::Lookahead {
public:
    NoLookahead(Decoder &decoder) { set_arc_la_updates(); };
    ~NoLookahead() {};
    float get_lookahead_score(int node_idx, int word_id) { return 0.0; }
    void set_arc_la_updates();
};


class UnigramLookahead : public Decoder::Lookahead {
public:
    UnigramLookahead(Decoder &decoder,
                     std::string lafname);
    ~UnigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

private:
    int set_unigram_la_scores();
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


class FullTableBigramLookahead : public Decoder::Lookahead {
public:
    FullTableBigramLookahead(Decoder &decoder,
                             std::string lafname,
                             bool successor_lists=true);
    ~FullTableBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

protected:

    int set_la_state_indices_to_nodes();
    void propagate_la_state_idx(int node_idx,
                                int la_state_idx,
                                int &max_state_idx,
                                bool first_node=true);
    int set_la_state_successor_lists();
    float set_arc_la_updates();

    std::vector<int> m_node_la_states;
    std::vector<std::vector<int> > m_la_state_successor_words;
    std::vector<std::vector<float> > m_bigram_la_scores;
};


class PrecomputedFullTableBigramLookahead : public FullTableBigramLookahead {
public:
    PrecomputedFullTableBigramLookahead(Decoder &decoder,
                                        std::string lafname);
    ~PrecomputedFullTableBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

private:
    void set_word_id_la_states();
    void convert_reverse_bigram_idxs(std::map<int, std::vector<int> > &reverse_bigrams);
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

    std::vector<int> m_word_id_la_state_lookup;
    bool m_precomputed;
};


// Full bigram tables for cross-word and initial nodes
// Map style data structures in other nodes
class HybridBigramLookahead : public Decoder::Lookahead {
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
};


class PrecomputedHybridBigramLookahead : public HybridBigramLookahead {
public:
    PrecomputedHybridBigramLookahead(Decoder &decoder,
                                     std::string lafname);
    ~PrecomputedHybridBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

private:
    void set_word_id_la_states();
    void convert_reverse_bigram_idxs(std::map<int, std::vector<int> > &reverse_bigrams);
    void find_preceeding_la_states(int node_idx,
                                   std::set<int> &la_states,
                                   const std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                   bool first_node=true,
                                   bool state_change=true);
    void find_first_lm_nodes(std::set<int> &lm_nodes,
                             int node_idx=START_NODE);

    void set_unigram_la_scores();
    void propagate_unigram_la_score(int node_idx,
                                    float score,
                                    int word_id,
                                    std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                    std::vector<std::pair<int, float> > &unigram_la_scores,
                                    bool start_node);
    void set_bigram_la_scores();

    std::vector<int> m_word_id_la_state_lookup;
    bool m_precomputed;
};


class BigramScoreLookahead : public Decoder::Lookahead {
public:
    BigramScoreLookahead(Decoder &decoder,
                         std::string lafname);
    ~BigramScoreLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

private:
    void set_bigram_la_scores_to_hmm_nodes();
    void set_bigram_la_scores_to_lm_nodes();
    float set_arc_la_updates();

    std::vector<float> m_la_scores;
};


class FullTableBigramLookahead2 : public Decoder::Lookahead {
public:
    FullTableBigramLookahead2(Decoder &decoder,
                              std::string lafname);
    ~FullTableBigramLookahead2() {};
    float get_lookahead_score(int node_idx, int word_id);

private:

    int set_one_predecessor_la_scores();
    int set_la_state_indices_to_nodes();
    int set_la_state_successor_lists();
    float set_arc_la_updates();

    std::vector<int> m_node_la_states;
    std::vector<std::vector<int> > m_la_state_successor_words;
    std::vector<std::vector<float> > m_bigram_la_scores;
    std::vector<float> m_one_predecessor_la_scores;
};


class LargeBigramLookahead : public Decoder::Lookahead {
public:
    LargeBigramLookahead(Decoder &decoder,
                         std::string lafname);
    ~LargeBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);
    void write(std::string ofname);
    void read(std::string ifname);

private:

    int set_la_state_indices_to_nodes();
    float set_arc_la_updates();
    void set_word_id_la_states();
    void propagate_la_state_idx(int node_idx,
                                int la_state_idx,
                                int &max_state_idx,
                                bool first_node=true);
    void find_preceeding_la_states(int node_idx,
                                   std::set<int> &la_states,
                                   const std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                   bool first_node=true,
                                   bool state_change=true);
    void convert_reverse_bigram_idxs(std::map<int, std::vector<int> > &reverse_bigrams);

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
    std::vector<int> m_word_id_la_state_lookup;
};


#endif /* LOOKAHEAD_HH */
