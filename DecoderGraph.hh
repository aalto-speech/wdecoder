#ifndef DECODERGRAPH_HH
#define DECODERGRAPH_HH

#include <map>
#include <deque>
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
        // Cross-word book-keeping used in construction
        std::set<std::string> from_fanin;
        std::set<std::string> to_fanout;

        int find_next(int word_id, int hmm_state) {
            if (lookahead == nullptr) throw std::string("Lookahead not set.");
            std::pair<int,int> id = std::make_pair(word_id, hmm_state);
            if (lookahead->find(id) != lookahead->end())
                return (*lookahead)[id];
            else return -1;
        }
    };

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

    int m_states_per_phone;

    std::vector<DecoderGraph::Node> m_nodes;


    DecoderGraph() { m_states_per_phone = -1;
                     m_nodes.resize(2); }
    virtual ~DecoderGraph() { };

    void read_phone_model(std::string phnfname);
    void read_noway_lexicon(std::string lexfname);


    void read_words(std::string wordfname,
                    std::set<std::string> &words) const;

    void read_word_segmentations(std::string segfname,
                                 std::map<std::string, std::vector<std::string> > &word_segs) const;

    // Static and const functions for triphonizing subwords and words

    static char tlc(std::string triphone);
    static char trc(std::string triphone);
    static char tphone(std::string triphone);
    static bool is_triphone(std::string triphone);

    static std::string construct_triphone(std::string left_ctxt,
                                          std::string phone,
                                          std::string right_ctxt);
    static std::string construct_triphone(char left_ctxt,
                                          char phone,
                                          char right_ctxt);
    static void triphonize_phone_string(std::string pstring,
                                        std::vector<std::string> &triphones);
    void triphonize(std::map<std::string, std::vector<std::string> > &word_segs,
                    std::string word,
                    std::vector<std::string> &triphones) const;
    bool triphonize(const std::vector<std::string> &word_seg,
                    std::vector<TriphoneNode> &nodes) const;
    void triphonize(std::string word,
                    std::vector<std::string> &triphones) const;
    void triphonize_subword(const std::string &subword,
                            std::vector<TriphoneNode> &nodes) const;
    void triphones_to_state_chain(std::vector<TriphoneNode> &triphone_nodes,
                                  std::vector<DecoderGraph::Node> &nodes) const;

    void add_nodes_to_tree(std::vector<DecoderGraph::Node> &nodes,
                           std::vector<DecoderGraph::Node> &new_nodes);
    void lookahead_to_arcs(std::vector<DecoderGraph::Node> &nodes);

    void get_hmm_states(const std::vector<std::string> &triphones,
                        std::vector<int> &states) const;
    void get_hmm_states(std::map<std::string, std::vector<std::string> > &word_segs,
                        std::string word,
                        std::vector<int> &states) const;
    void get_hmm_states_cw(std::map<std::string, std::vector<std::string> > &word_segs,
                           std::string wrd1,
                           std::string wrd2,
                           std::vector<int> &states) const;

    void find_successor_word(std::vector<DecoderGraph::Node> &nodes,
                             std::set<std::pair<int, int> > &matches,
                             int word_id,
                             node_idx_t node_idx=START_NODE,
                             int depth=0);
    void find_nodes_in_depth(std::vector<DecoderGraph::Node> &nodes,
                             std::set<node_idx_t> &found_nodes,
                             int target_depth,
                             int curr_depth=0,
                             node_idx_t curr_node=START_NODE);
    void find_nodes_in_depth_reverse(std::vector<DecoderGraph::Node> &nodes,
                                     std::set<node_idx_t> &found_nodes,
                                     int target_depth,
                                     int curr_depth=0,
                                     node_idx_t curr_node=END_NODE);
    bool assert_path(std::vector<DecoderGraph::Node> &nodes,
                     std::deque<int> states,
                     std::deque<std::string> subwords,
                     node_idx_t node_idx);
    bool assert_path(std::vector<DecoderGraph::Node> &nodes,
                     std::vector<std::string> &triphones,
                     std::vector<std::string> &subwords);
    bool assert_transitions(std::vector<DecoderGraph::Node> &nodes);
    bool assert_words(std::map<std::string, std::vector<std::string> > &word_segs);
    bool assert_words_not_in_graph(std::map<std::string, std::vector<std::string> > &word_segs);
    bool assert_words(std::set<std::string> &words);
    bool assert_word_pair_crossword(std::vector<DecoderGraph::Node> &nodes,
                                    std::map<std::string, std::vector<std::string> > &word_segs,
                                    std::string word1,
                                    std::string word2,
                                    bool short_silence=true,
                                    bool wb_symbol=false);
    bool assert_word_pairs(std::map<std::string, std::vector<std::string> > &word_segs,
                           bool short_silence=true,
                           bool wb_symbol=false);
    bool assert_word_pairs(std::set<std::string> &words,
                           bool short_silence=true,
                           bool wb_symbol=false);
    bool assert_word_pairs(std::map<std::string, std::vector<std::string> > &word_segs,
                           int num_pairs,
                           bool short_silence=true,
                           bool wb_symbol=false);
    bool assert_word_pairs(std::set<std::string> &words,
                           int num_pairs,
                           bool short_silence=true,
                           bool wb_symbol=false);
    static bool assert_subword_ids_left(std::vector<DecoderGraph::Node> &nodes);
    static bool assert_subword_ids_right(std::vector<DecoderGraph::Node> &nodes);
    bool assert_no_duplicate_word_ids();
    bool assert_only_segmented_words(std::map<std::string, std::vector<std::string> > &word_segs,
                                     std::deque<int> states = std::deque<int>(),
                                     std::deque<int> subwords = std::deque<int>(),
                                     int node_idx=START_NODE);
    bool assert_only_words(std::set<std::string> &words);
    bool assert_only_segmented_cw_word_pairs(
            std::map<std::string, std::vector<std::string> > &word_segs,
            std::vector<int> states = std::vector<int>(),
            std::pair<std::vector<int>, std::vector<int>> subwords = std::pair<std::vector<int>, std::vector<int>>(),
            int node_idx = START_NODE,
            bool cw_visited = false);
    bool assert_only_cw_word_pairs(std::set<std::string> &words);

    static void tie_state_prefixes(std::vector<DecoderGraph::Node> &nodes,
                                   bool stop_propagation=false,
                                   node_idx_t node_idx=START_NODE);
    static void tie_state_prefixes_cw(std::vector<DecoderGraph::Node> &cw_nodes,
                                      std::map<std::string, int> &fanout,
                                      std::map<std::string, int> &fanin,
                                      bool stop_propagation=false);
    static void tie_state_prefixes(std::vector<DecoderGraph::Node> &nodes,
                                   std::set<node_idx_t> &processed_nodes,
                                   bool stop_propagation=false,
                                   node_idx_t node_idx=START_NODE);

    static void tie_state_suffixes(std::vector<DecoderGraph::Node> &nodes,
                                   bool stop_propagation=false,
                                   node_idx_t node_idx=END_NODE);
    static void tie_state_suffixes_cw(std::vector<DecoderGraph::Node> &cw_nodes,
                                      std::map<std::string, int> &fanout,
                                      std::map<std::string, int> &fanin,
                                      bool stop_propagation=false);
    static void tie_state_suffixes(std::vector<DecoderGraph::Node> &nodes,
                                   std::set<node_idx_t> &processed_nodes,
                                   bool stop_propagation=false,
                                   node_idx_t node_idx=END_NODE);

    static void tie_word_id_prefixes(std::vector<DecoderGraph::Node> &nodes,
                                     bool stop_propagation=false,
                                     node_idx_t node_idx=START_NODE);
    static void tie_word_id_prefixes_cw(std::vector<DecoderGraph::Node> &cw_nodes,
                                        std::map<std::string, int> &fanout,
                                        std::map<std::string, int> &fanin,
                                        bool stop_propagation=false);
    static void tie_word_id_prefixes(std::vector<DecoderGraph::Node> &nodes,
                                     std::set<node_idx_t> &processed_nodes,
                                     bool stop_propagation=false,
                                     node_idx_t node_idx=START_NODE);

    static void tie_word_id_suffixes(std::vector<DecoderGraph::Node> &nodes,
                                     bool stop_propagation=false,
                                     node_idx_t node_idx=END_NODE);
    static void tie_word_id_suffixes_cw(std::vector<DecoderGraph::Node> &cw_nodes,
                                        std::map<std::string, int> &fanout,
                                        std::map<std::string, int> &fanin,
                                        bool stop_propagation=false);
    static void tie_word_id_suffixes(std::vector<DecoderGraph::Node> &nodes,
                                     std::set<node_idx_t> &processed_nodes,
                                     bool stop_propagation=false,
                                     node_idx_t node_idx=END_NODE);

    void minimize_crossword_network(std::vector<DecoderGraph::Node> &cw_nodes,
                                    std::map<std::string, int> &fanout,
                                    std::map<std::string, int> &fanin);

    void connect_crossword_network(std::vector<DecoderGraph::Node> &cw_nodes,
                                   std::map<std::string, int> &fanout,
                                   std::map<std::string, int> &fanin,
                                   bool push_left_after_fanin=true);

    void print_graph(std::vector<DecoderGraph::Node> &nodes,
                     std::vector<int> path,
                     int node_idx);
    void print_graph(std::vector<DecoderGraph::Node> &nodes);
    void print_dot_digraph(std::vector<DecoderGraph::Node> &nodes,
                           std::ostream &fstr = std::cout,
                           bool mark_start_end = true);

    static void set_reverse_arcs_also_from_unreachable(std::vector<DecoderGraph::Node> &nodes);
    static void set_reverse_arcs(std::vector<DecoderGraph::Node> &nodes);
    static void set_reverse_arcs(std::vector<DecoderGraph::Node> &nodes,
                          int node_idx,
                          std::set<int> &processed_nodes);
    static void clear_reverse_arcs(std::vector<DecoderGraph::Node> &nodes);
    static int merge_nodes(std::vector<DecoderGraph::Node> &nodes, int node_idx_1, int node_idx_2);
    void connect_end_to_start_node(std::vector<DecoderGraph::Node> &nodes);

    int connect_triphone(std::vector<DecoderGraph::Node> &nodes,
                         std::string triphone,
                         node_idx_t node_idx,
                         int flag_mask=0);
    int connect_triphone(std::vector<DecoderGraph::Node> &nodes,
                         int triphone_idx,
                         node_idx_t node_idx,
                         int flag_mask=0);
    int connect_triphone(std::vector<DecoderGraph::Node> &nodes,
                         std::string triphone,
                         node_idx_t node_idx,
                         std::map<int, std::string> &node_labels,
                         int flag_mask=0);
    int connect_triphone(std::vector<DecoderGraph::Node> &nodes,
                         int triphone_idx,
                         node_idx_t node_idx,
                         std::map<int, std::string> &node_labels,
                         int flag_mask=0);
    int connect_word(std::vector<DecoderGraph::Node> &nodes,
                     std::string word,
                     node_idx_t node_idx,
                     int flag_mask=0);
    int connect_word(std::vector<DecoderGraph::Node> &nodes,
                     int word_id,
                     node_idx_t node_idx,
                     int flag_mask=0);
    int connect_dummy(std::vector<DecoderGraph::Node> &nodes,
                      node_idx_t node_idx,
                      int flag_mask=0);

    static int reachable_graph_nodes(std::vector<DecoderGraph::Node> &nodes);
    static void reachable_graph_nodes(std::vector<DecoderGraph::Node> &nodes,
                               std::set<node_idx_t> &node_idxs,
                               const std::set<node_idx_t> &start_nodes);
    static void reachable_graph_nodes(std::vector<DecoderGraph::Node> &nodes,
                               std::set<node_idx_t> &node_idxs,
                               node_idx_t node_idx=START_NODE);
    static void prune_unreachable_nodes(std::vector<DecoderGraph::Node> &nodes);
    static void prune_unreachable_nodes_cw(std::vector<DecoderGraph::Node> &nodes,
                                           const std::set<node_idx_t> &start_nodes,
                                           std::map<std::string, int> &fanout,
                                           std::map<std::string, int> &fanin);
    static void push_word_ids_left(std::vector<DecoderGraph::Node> &nodes);
    static void push_word_ids_left(std::vector<DecoderGraph::Node> &nodes,
                                   int &move_count,
                                   std::set<node_idx_t> &processed_nodes,
                                   node_idx_t node_idx=END_NODE,
                                   node_idx_t prev_node_idx=END_NODE,
                                   int subword_id=-1);
    static void push_word_ids_right(std::vector<DecoderGraph::Node> &nodes);
    static void push_word_ids_right(std::vector<DecoderGraph::Node> &nodes,
                                    int &move_count,
                                    std::set<node_idx_t> &processed_nodes,
                                    node_idx_t node_idx=START_NODE,
                                    node_idx_t prev_node_idx=START_NODE,
                                    int subword_id=-1);
    static int num_hmm_states(std::vector<DecoderGraph::Node> &nodes);
    static int num_subword_states(std::vector<DecoderGraph::Node> &nodes);
    static int num_arcs(std::vector<DecoderGraph::Node> &nodes);
    static int num_nodes_with_no_arcs(std::vector<DecoderGraph::Node> &nodes);
    static int num_triphones(std::vector<TriphoneNode> &nodes);

    void add_hmm_self_transitions();
    void add_hmm_self_transitions(std::vector<DecoderGraph::Node> &nodes);
    void add_long_silence();
    void write_graph(std::string fname,
                     bool lm_labels=false);

    void remove_cw_dummies(std::vector<DecoderGraph::Node> &nodes);
    void remove_nodes_with_no_arcs(std::vector<DecoderGraph::Node> &nodes);

    virtual void create_forced_path(std::vector<DecoderGraph::Node> &nodes,
                                    std::vector<std::string> &sentence,
                                    std::map<int, std::string> &node_labels) {
        throw std::string("create_forced_path not defined here");
    }

};

#endif /* DECODERGRAPH_HH */
