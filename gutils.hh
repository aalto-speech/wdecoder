#ifndef GUTILS_HH
#define GUTILS_HH

#include <deque>
#include <map>
#include <string>
#include <set>
#include <vector>

#include "DecoderGraph.hh"


namespace gutils {

void triphonize(std::string word,
                std::vector<std::string> &triphones);
void triphonize(DecoderGraph &dg,
                std::string word,
                std::vector<std::string> &triphones);
void triphonize_all_words(DecoderGraph &dg,
                          std::map<std::string, std::vector<std::string> > &triphonized_words);
void get_hmm_states(DecoderGraph &dg,
                    const std::vector<std::string> &triphones,
                    std::vector<int> &states);
void get_hmm_states(DecoderGraph &dg,
                    std::string word,
                    std::vector<int> &states);
void get_hmm_states_cw(DecoderGraph &dg,
                       std::string wrd1,
                       std::string wrd2,
                       std::vector<int> &states);
bool assert_path(DecoderGraph &dg,
                 std::vector<DecoderGraph::Node> &nodes,
                 std::deque<int> states,
                 std::deque<std::string> subwords,
                 int node_idx);
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
                                std::string word1,
                                std::string word2,
                                bool debug);
bool assert_words(DecoderGraph &dg,
                  std::vector<DecoderGraph::Node> &nodes,
                  bool debug);
bool assert_words(DecoderGraph &dg,
                  std::vector<DecoderGraph::Node> &nodes,
                  std::map<std::string, std::vector<std::string> > &triphonized_words,
                  bool debug);
bool assert_word_pairs(DecoderGraph &dg,
                       std::vector<DecoderGraph::Node> &nodes,
                       bool debug);
bool assert_word_pairs(DecoderGraph &dg,
                       std::vector<DecoderGraph::Node> &nodes,
                       int num_pairs=10000,
                       bool debug=false);
bool assert_subword_ids_left(DecoderGraph &dg,
                             std::vector<DecoderGraph::Node> &nodes,
                             bool debug=false);
bool assert_subword_ids_right(DecoderGraph &dg,
                              std::vector<DecoderGraph::Node> &nodes,
                              bool debug=false);
bool assert_no_double_arcs(std::vector<DecoderGraph::Node> &nodes);
bool assert_prefix_state_tying(DecoderGraph &dg,
                               std::vector<DecoderGraph::Node> &nodes);
bool assert_suffix_state_tying(DecoderGraph &dg,
                               std::vector<DecoderGraph::Node> &nodes);
bool assert_only_segmented_words(DecoderGraph &dg,
                                 std::vector<DecoderGraph::Node> &nodes,
                                 std::deque<int> states = std::deque<int>(),
                                 std::deque<int> subwords = std::deque<int>(),
                                 int node_idx = DecoderGraph::START_NODE);
bool assert_only_segmented_cw_word_pairs(DecoderGraph &dg,
        std::vector<DecoderGraph::Node> &nodes,
        std::deque<int> states = std::deque<int>(),
        std::deque<int> subwords = std::deque<int>(),
        int node_idx = DecoderGraph::START_NODE,
        bool cw_visited = false);

};

#endif /* GUTILS_HH */