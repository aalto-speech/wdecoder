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


class FullTableBigramLookahead : public Decoder::Lookahead {
public:
    FullTableBigramLookahead(Decoder &decoder,
                             std::string lafname);
    ~FullTableBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

private:

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


// Full bigram tables for cross-word and initial nodes
// Map style data structures in other nodes
class HybridBigramLookahead : public Decoder::Lookahead {
public:
    HybridBigramLookahead(Decoder &decoder,
                          std::string lafname);
    ~HybridBigramLookahead() {};
    float get_lookahead_score(int node_idx, int word_id);

private:

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

private:

    int set_la_state_indices_to_nodes();
    int initialize_la_states();
    float set_arc_la_updates();
    void propagate_la_state_idx(int node_idx,
                                int la_state_idx,
                                int &max_state_idx,
                                bool first_node=true);
    void find_preceeding_la_states(int node_idx,
                                   std::set<int> &la_states,
                                   const std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                   bool first_node=true);
    void convert_reverse_bigram_idxs(std::map<int, std::vector<int> > &reverse_bigrams);

    void propagate_unigram_la_score(int node_idx,
                                    float score,
                                    int word_id,
                                    std::vector<std::vector<Decoder::Arc> > &reverse_arcs,
                                    int &la_score_set,
                                    bool start_node);
    int set_unigram_la_scores();

    int set_bigram_la_scores();

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
