#ifndef DECODER_HH
#define DECODER_HH

#include <map>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <vector>
#include <set>

#include "Hmm.hh"
#include "LM.hh"
#include "LnaReaderCircular.hh"


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

    class WordHistory {
    public:
        WordHistory() { }
        WordHistory(int word_id, std::shared_ptr<WordHistory> previous)
            : word_id(word_id), previous(previous) { }
        int word_id;
        std::shared_ptr<WordHistory> previous;
        std::unordered_map<int, std::weak_ptr<WordHistory> > next;
    };

    class Token {
    public:
      int node_idx;
      float am_log_prob;
      float cur_am_log_prob;
      float lm_log_prob;
      float total_log_prob;
      int fsa_lm_node;
      int word_start_frame;
      unsigned short int dur;
      std::shared_ptr<WordHistory> history;

      Token():
        node_idx(-1),
        am_log_prob(0.0f),
        cur_am_log_prob(0.0f),
        lm_log_prob(0.0f),
        total_log_prob(0.0f),
        fsa_lm_node(0),
        word_start_frame(0),
        dur(0)
      { }
    };

    Decoder() {
        debug = 0;

        m_lm_scale = 0.0;
        m_duration_scale = 0.0;
        m_transition_scale = 0.0;
        m_max_num_tokens = 0;

        m_global_beam = 0.0;
        m_current_glob_beam = 0.0;
        m_state_beam = 0.0;
    };

    void read_phone_model(std::string phnfname);
    void read_duration_model(std::string durfname);
    void read_noway_lexicon(std::string lexfname);
    void read_lm(std::string lmfname);
    void read_dgraph(std::string graphfname);

    void set_lm_scale(float lm_scale) { m_lm_scale = lm_scale; }
    void set_duration_scale(float dur_scale) { m_duration_scale = dur_scale; }
    void set_transition_scale(float trans_scale) { m_transition_scale = trans_scale; }
    void set_max_num_tokens(int tokens) { m_max_num_tokens = tokens; }
    void set_state_beam(float beam) { m_state_beam = beam; }
    void set_global_beam(float beam) { m_global_beam = beam; }

    void recognize_lna_file(std::string &lnafname);
    void initialize();
    void propagate_tokens();
    void move_token_to_node(Token token, int node_idx, float transition_score);

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
    // Language model
    fsalm::LM m_lm;
    // Audio reader
    LnaReaderCircular m_lna_reader;

    std::vector<Node> m_nodes;
    std::vector<Token> m_tokens;

private:

    void add_silence_hmms(std::vector<Node> &nodes,
                          bool long_silence=true,
                          bool short_silence=false);
    void add_hmm_self_transitions(std::vector<Node> &nodes);
    void set_hmm_transition_probs(std::vector<Node> &nodes);

    float m_lm_scale;
    float m_duration_scale;
    float m_transition_scale; // Temporary scaling used for self transitions
    int m_max_num_tokens;

    float m_global_beam;
    float m_current_glob_beam;
    float m_state_beam;

    float m_best_log_prob;
    float m_worst_log_prob;
};

#endif /* DECODER_HH */
