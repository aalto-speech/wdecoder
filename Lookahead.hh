#ifndef LOOKAHEAD_HH
#define LOOKAHEAD_HH

#include <vector>

#include "Decoder.hh"


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

    std::vector<int> m_node_la_states;
    std::vector<std::vector<int> > m_la_state_successor_words;
    std::vector<std::vector<float> > m_bigram_la_scores;
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
    std::vector<float> m_la_scores;
};


#endif /* LOOKAHEAD_HH */
