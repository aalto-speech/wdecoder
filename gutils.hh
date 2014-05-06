#ifndef GUTILS_HH
#define GUTILS_HH

#include <deque>
#include <map>
#include <string>
#include <set>
#include <vector>

#include "defs.hh"
#include "DecoderGraph.hh"


namespace gutils {

static void triphonize(std::string word,
                std::vector<std::string> &triphones);
static void triphonize(DecoderGraph &dg,
                std::string word,
                std::vector<std::string> &triphones);
static void triphonize(DecoderGraph &dg,
                std::vector<std::string> &word_seg,
                std::vector<DecoderGraph::TriphoneNode> &nodes);
static void triphonize_all_words(DecoderGraph &dg,
                          std::map<std::string, std::vector<std::string> > &triphonized_words);
static void triphonize_subword(DecoderGraph &dg,
                        const std::string &subword,
                        std::vector<DecoderGraph::TriphoneNode> &nodes);
static void get_hmm_states(DecoderGraph &dg,
                    const std::vector<std::string> &triphones,
                    std::vector<int> &states);
static void get_hmm_states(DecoderGraph &dg,
                    std::string word,
                    std::vector<int> &states);
static void get_hmm_states_cw(DecoderGraph &dg,
                       std::string wrd1,
                       std::string wrd2,
                       std::vector<int> &states);
static void find_successor_word(std::vector<DecoderGraph::Node> &nodes,
                         std::set<std::pair<int, int> > &matches,
                         int word_id,
                         node_idx_t node_idx=START_NODE,
                         int depth=0);
static bool assert_path(DecoderGraph &dg,
                 std::vector<DecoderGraph::Node> &nodes,
                 std::deque<int> states,
                 std::deque<std::string> subwords,
                 node_idx_t node_idx);
static bool assert_path(DecoderGraph &dg,
                 std::vector<DecoderGraph::Node> &nodes,
                 std::vector<std::string> &triphones,
                 std::vector<std::string> &subwords,
                 bool debug);
static bool assert_transitions(DecoderGraph &dg,
                        std::vector<DecoderGraph::Node> &nodes,
                        bool debug);
static bool assert_word_pair_crossword(DecoderGraph &dg,
                                std::vector<DecoderGraph::Node> &nodes,
                                std::string word1,
                                std::string word2,
                                bool debug);
static bool assert_words(DecoderGraph &dg,
                  std::vector<DecoderGraph::Node> &nodes,
                  bool debug);
static bool assert_words(DecoderGraph &dg,
                  std::vector<DecoderGraph::Node> &nodes,
                  std::map<std::string, std::vector<std::string> > &triphonized_words,
                  bool debug);
static bool assert_word_pairs(DecoderGraph &dg,
                       std::vector<DecoderGraph::Node> &nodes,
                       bool debug);
static bool assert_word_pairs(DecoderGraph &dg,
                       std::vector<DecoderGraph::Node> &nodes,
                       int num_pairs=10000,
                       bool debug=false);
static bool assert_subword_ids_left(DecoderGraph &dg,
                             std::vector<DecoderGraph::Node> &nodes,
                             bool debug=false);
static bool assert_subword_ids_right(DecoderGraph &dg,
                              std::vector<DecoderGraph::Node> &nodes,
                              bool debug=false);
static bool assert_no_double_arcs(std::vector<DecoderGraph::Node> &nodes);
static bool assert_no_duplicate_word_ids(DecoderGraph &dg,
                                  std::vector<DecoderGraph::Node> &nodes);
static bool assert_only_segmented_words(DecoderGraph &dg,
                                 std::vector<DecoderGraph::Node> &nodes,
                                 bool debug=false,
                                 std::deque<int> states = std::deque<int>(),
                                 std::deque<int> subwords = std::deque<int>(),
                                 int node_idx=START_NODE);
static bool assert_only_segmented_cw_word_pairs(DecoderGraph &dg,
        std::vector<DecoderGraph::Node> &nodes,
        std::deque<int> states = std::deque<int>(),
        std::deque<int> subwords = std::deque<int>(),
        int node_idx = START_NODE,
        bool cw_visited = false);

static void tie_state_prefixes(std::vector<DecoderGraph::Node> &nodes,
                               bool stop_propagation=false);
static void tie_state_prefixes(std::vector<DecoderGraph::Node> &nodes,
                        std::set<node_idx_t> &processed_nodes,
                        bool stop_propagation=false,
                        node_idx_t node_idx=START_NODE);
static void tie_state_suffixes(std::vector<DecoderGraph::Node> &nodes,
                        bool stop_propagation=false);
static void tie_state_suffixes(std::vector<DecoderGraph::Node> &nodes,
                        std::set<node_idx_t> &processed_nodes,
                        bool stop_propagation=false,
                        node_idx_t node_idx=END_NODE);
static void tie_word_id_prefixes(std::vector<DecoderGraph::Node> &nodes,
                          bool stop_propagation=false);
static void tie_word_id_prefixes(std::vector<DecoderGraph::Node> &nodes,
                          std::set<node_idx_t> &processed_nodes,
                          bool stop_propagation=false,
                          node_idx_t node_idx=START_NODE);
static void tie_word_id_suffixes(std::vector<DecoderGraph::Node> &nodes,
                          bool stop_propagation=false);
static void tie_word_id_suffixes(std::vector<DecoderGraph::Node> &nodes,
                          std::set<node_idx_t> &processed_nodes,
                          bool stop_propagation=false,
                          node_idx_t node_idx=END_NODE);

static void print_graph(std::vector<DecoderGraph::Node> &nodes,
                 std::vector<int> path,
                 int node_idx);
static void print_graph(std::vector<DecoderGraph::Node> &nodes);
static void print_dot_digraph(std::vector<DecoderGraph::Node> &nodes, std::ostream &fstr = std::cout);
static int reachable_graph_nodes(std::vector<DecoderGraph::Node> &nodes);

static void set_reverse_arcs_also_from_unreachable(std::vector<DecoderGraph::Node> &nodes);
static void set_reverse_arcs(std::vector<DecoderGraph::Node> &nodes);
static void set_reverse_arcs(std::vector<DecoderGraph::Node> &nodes,
                      int node_idx,
                      std::set<int> &processed_nodes);
static void clear_reverse_arcs(std::vector<DecoderGraph::Node> &nodes);
static int merge_nodes(std::vector<DecoderGraph::Node> &nodes, int node_idx_1, int node_idx_2);
static void connect_end_to_start_node(std::vector<DecoderGraph::Node> &nodes);
static void write_graph(std::vector<DecoderGraph::Node> &nodes, std::string fname);

static int connect_triphone(DecoderGraph &dg,
                    std::vector<DecoderGraph::Node> &nodes,
                     std::string triphone,
                     node_idx_t node_idx);
static int connect_triphone(DecoderGraph &dg,
                    std::vector<DecoderGraph::Node> &nodes,
                     int triphone_idx,
                     node_idx_t node_idx);
static int connect_word(DecoderGraph &dg,
                std::vector<DecoderGraph::Node> &nodes,
                 std::string word,
                 node_idx_t node_idx);

static void reachable_graph_nodes(std::vector<DecoderGraph::Node> &nodes,
                           std::set<node_idx_t> &node_idxs,
                           node_idx_t node_idx=START_NODE);
static void prune_unreachable_nodes(std::vector<DecoderGraph::Node> &nodes);
static void add_hmm_self_transitions(std::vector<DecoderGraph::Node> &nodes);
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



};

#endif /* GUTILS_HH */
