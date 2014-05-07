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

void read_word_segmentations(DecoderGraph &dg,
                             std::string segfname,
                             std::vector<std::pair<std::string, std::vector<std::string> > > &word_segs);

void read_word_segmentations(DecoderGraph &dg,
                             std::string segfname,
                             std::map<std::string, std::vector<std::string> > &word_segs);

void triphonize(std::string word,
                std::vector<std::string> &triphones);
void triphonize(DecoderGraph &dg,
                std::map<std::string, std::vector<std::string> > &word_segs,
                std::string word,
                std::vector<std::string> &triphones);
void triphonize(DecoderGraph &dg,
                std::vector<std::string> &word_seg,
                std::vector<DecoderGraph::TriphoneNode> &nodes);
void triphonize_all_words(DecoderGraph &dg,
                          std::map<std::string, std::vector<std::string> > &word_segs,
                          std::map<std::string, std::vector<std::string> > &triphonized_words);
void triphonize_subword(DecoderGraph &dg,
                        const std::string &subword,
                        std::vector<DecoderGraph::TriphoneNode> &nodes);
void get_hmm_states(DecoderGraph &dg,
                    const std::vector<std::string> &triphones,
                    std::vector<int> &states);
void get_hmm_states(DecoderGraph &dg,
                    std::string word,
                    std::vector<int> &states);
void get_hmm_states_cw(DecoderGraph &dg,
                       std::map<std::string, std::vector<std::string> > &word_segs,
                       std::string wrd1,
                       std::string wrd2,
                       std::vector<int> &states);
void find_successor_word(std::vector<DecoderGraph::Node> &nodes,
                         std::set<std::pair<int, int> > &matches,
                         int word_id,
                         node_idx_t node_idx=START_NODE,
                         int depth=0);
bool assert_path(DecoderGraph &dg,
                 std::vector<DecoderGraph::Node> &nodes,
                 std::deque<int> states,
                 std::deque<std::string> subwords,
                 node_idx_t node_idx);
bool assert_path(DecoderGraph &dg,
                 std::vector<DecoderGraph::Node> &nodes,
                 std::vector<std::string> &triphones,
                 std::vector<std::string> &subwords,
                 bool debug);
bool assert_transitions(DecoderGraph &dg,
                        std::vector<DecoderGraph::Node> &nodes,
                        bool debug);
bool assert_word_pair_crossword(DecoderGraph &dg,
                                std::vector<DecoderGraph::Node> &nodes,
                                std::map<std::string, std::vector<std::string> > &word_segs,
                                std::string word1,
                                std::string word2,
                                bool debug);
bool assert_words(DecoderGraph &dg,
                  std::vector<DecoderGraph::Node> &nodes,
                  std::map<std::string, std::vector<std::string> > &word_segs,
                  bool debug);
bool assert_words(DecoderGraph &dg,
                  std::vector<DecoderGraph::Node> &nodes,
                  std::map<std::string, std::vector<std::string> > &word_segs,
                  std::map<std::string, std::vector<std::string> > &triphonized_words,
                  bool debug);
bool assert_word_pairs(DecoderGraph &dg,
                       std::vector<DecoderGraph::Node> &nodes,
                       std::map<std::string, std::vector<std::string> > &word_segs,
                       bool debug);
bool assert_word_pairs(DecoderGraph &dg,
                       std::vector<DecoderGraph::Node> &nodes,
                       std::map<std::string, std::vector<std::string> > &word_segs,
                       int num_pairs=10000,
                       bool debug=false);
bool assert_subword_ids_left(DecoderGraph &dg,
                             std::vector<DecoderGraph::Node> &nodes,
                             bool debug=false);
bool assert_subword_ids_right(DecoderGraph &dg,
                              std::vector<DecoderGraph::Node> &nodes,
                              bool debug=false);
bool assert_no_double_arcs(std::vector<DecoderGraph::Node> &nodes);
bool assert_no_duplicate_word_ids(DecoderGraph &dg,
                                  std::vector<DecoderGraph::Node> &nodes);
bool assert_only_segmented_words(DecoderGraph &dg,
                                 std::vector<DecoderGraph::Node> &nodes,
                                 std::map<std::string, std::vector<std::string> > &word_segs,
                                 bool debug=false,
                                 std::deque<int> states = std::deque<int>(),
                                 std::deque<int> subwords = std::deque<int>(),
                                 int node_idx=START_NODE);
bool assert_only_segmented_cw_word_pairs(DecoderGraph &dg,
        std::vector<DecoderGraph::Node> &nodes,
        std::map<std::string, std::vector<std::string> > &word_segs,
        std::deque<int> states = std::deque<int>(),
        std::deque<int> subwords = std::deque<int>(),
        int node_idx = START_NODE,
        bool cw_visited = false);

void tie_state_prefixes(std::vector<DecoderGraph::Node> &nodes,
                        bool stop_propagation=false);
void tie_state_prefixes(std::vector<DecoderGraph::Node> &nodes,
                        std::set<node_idx_t> &processed_nodes,
                        bool stop_propagation=false,
                        node_idx_t node_idx=START_NODE);
void tie_state_suffixes(std::vector<DecoderGraph::Node> &nodes,
                        bool stop_propagation=false);
void tie_state_suffixes(std::vector<DecoderGraph::Node> &nodes,
                        std::set<node_idx_t> &processed_nodes,
                        bool stop_propagation=false,
                        node_idx_t node_idx=END_NODE);
void tie_word_id_prefixes(std::vector<DecoderGraph::Node> &nodes,
                          bool stop_propagation=false);
void tie_word_id_prefixes(std::vector<DecoderGraph::Node> &nodes,
                          std::set<node_idx_t> &processed_nodes,
                          bool stop_propagation=false,
                          node_idx_t node_idx=START_NODE);
void tie_word_id_suffixes(std::vector<DecoderGraph::Node> &nodes,
                          bool stop_propagation=false);
void tie_word_id_suffixes(std::vector<DecoderGraph::Node> &nodes,
                          std::set<node_idx_t> &processed_nodes,
                          bool stop_propagation=false,
                          node_idx_t node_idx=END_NODE);

void print_graph(DecoderGraph &dg,
                 std::vector<DecoderGraph::Node> &nodes,
                 std::vector<int> path,
                 int node_idx);
void print_graph(DecoderGraph &dg,
                 std::vector<DecoderGraph::Node> &nodes);
void print_dot_digraph(DecoderGraph &dg,
                       std::vector<DecoderGraph::Node> &nodes,
                       std::ostream &fstr = std::cout);
int reachable_graph_nodes(std::vector<DecoderGraph::Node> &nodes);

void set_reverse_arcs_also_from_unreachable(std::vector<DecoderGraph::Node> &nodes);
void set_reverse_arcs(std::vector<DecoderGraph::Node> &nodes);
void set_reverse_arcs(std::vector<DecoderGraph::Node> &nodes,
                      int node_idx,
                      std::set<int> &processed_nodes);
void clear_reverse_arcs(std::vector<DecoderGraph::Node> &nodes);
int merge_nodes(std::vector<DecoderGraph::Node> &nodes, int node_idx_1, int node_idx_2);
void connect_end_to_start_node(std::vector<DecoderGraph::Node> &nodes);
void write_graph(std::vector<DecoderGraph::Node> &nodes, std::string fname);

int connect_triphone(DecoderGraph &dg,
                     std::vector<DecoderGraph::Node> &nodes,
                     std::string triphone,
                     node_idx_t node_idx);
int connect_triphone(DecoderGraph &dg,
                     std::vector<DecoderGraph::Node> &nodes,
                     int triphone_idx,
                     node_idx_t node_idx);
int connect_word(DecoderGraph &dg,
                 std::vector<DecoderGraph::Node> &nodes,
                 std::string word,
                 node_idx_t node_idx);

void reachable_graph_nodes(std::vector<DecoderGraph::Node> &nodes,
                           std::set<node_idx_t> &node_idxs,
                           node_idx_t node_idx=START_NODE);
void prune_unreachable_nodes(std::vector<DecoderGraph::Node> &nodes);
void add_hmm_self_transitions(std::vector<DecoderGraph::Node> &nodes);
void push_word_ids_left(std::vector<DecoderGraph::Node> &nodes);
void push_word_ids_left(std::vector<DecoderGraph::Node> &nodes,
                        int &move_count,
                        std::set<node_idx_t> &processed_nodes,
                        node_idx_t node_idx=END_NODE,
                        node_idx_t prev_node_idx=END_NODE,
                        int subword_id=-1);
void push_word_ids_right(std::vector<DecoderGraph::Node> &nodes);
void push_word_ids_right(std::vector<DecoderGraph::Node> &nodes,
                         int &move_count,
                         std::set<node_idx_t> &processed_nodes,
                         node_idx_t node_idx=START_NODE,
                         node_idx_t prev_node_idx=START_NODE,
                         int subword_id=-1);
int num_hmm_states(std::vector<DecoderGraph::Node> &nodes);
int num_subword_states(std::vector<DecoderGraph::Node> &nodes);

void add_triphones(std::vector<DecoderGraph::TriphoneNode> &nodes,
                   std::vector<DecoderGraph::TriphoneNode> &nodes_to_add);
void triphones_to_states(DecoderGraph &dg,
                         std::vector<DecoderGraph::TriphoneNode> &triphone_nodes,
                         std::vector<DecoderGraph::Node> &nodes,
                         int curr_triphone_idx=START_NODE,
                         int curr_state_idx=START_NODE);

void collect_cw_fanout_nodes(DecoderGraph &dg,
                             std::vector<DecoderGraph::Node> &nodes,
                             std::map<int, std::string> &nodes_to_fanout,
                             int hmm_state_count=0,
                             std::vector<char> phones = std::vector<char>(),
                             int node_to_connect=-1,
                             int node_idx=END_NODE);
void collect_cw_fanin_nodes(DecoderGraph &dg,
                            std::vector<DecoderGraph::Node> &nodes,
                            std::map<node_idx_t, std::string> &nodes_from_fanin,
                            int hmm_state_count=0,
                            std::vector<char> phones = std::vector<char>(),
                            node_idx_t node_to_connect=START_NODE,
                            node_idx_t node_idx=START_NODE);

}

#endif /* GUTILS_HH */
