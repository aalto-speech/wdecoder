#include <string>
#include <iostream>
#include <fstream>

#include "graphtest.hh"
#include "gutils.hh"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace std;
using namespace gutils;


CPPUNIT_TEST_SUITE_REGISTRATION (graphtest);


void graphtest::setUp (void)
{
    amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
    lexname = string("data/lex");
    segname = string("data/segs.txt");
}


void graphtest::tearDown (void)
{
}


void graphtest::read_fixtures(DecoderGraph &dg)
{
    dg.read_phone_model(amname + ".ph");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);
}


// Verify that models are correctly loaded
// Test constructing the initial word graph on subword level
void graphtest::GraphTest1(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    CPPUNIT_ASSERT_EQUAL( 35003, (int)dg.m_units.size() );
    CPPUNIT_ASSERT_EQUAL( 35003, (int)dg.m_unit_map.size() ); // ?
    CPPUNIT_ASSERT_EQUAL( 13252, (int)dg.m_hmms.size() );
    CPPUNIT_ASSERT_EQUAL( 13252, (int)dg.m_hmm_map.size() );
    CPPUNIT_ASSERT_EQUAL( 1170, (int)dg.m_hmm_states.size() );
    CPPUNIT_ASSERT_EQUAL( 11, (int)dg.m_word_segs.size() );

    vector<DecoderGraph::SubwordNode> nodes;
    dg.create_word_graph(nodes);
    CPPUNIT_ASSERT_EQUAL( 13, dg.reachable_word_graph_nodes(nodes) );
}


// Test tying word suffixes
void graphtest::GraphTest2(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> nodes;
    dg.create_word_graph(nodes);
    CPPUNIT_ASSERT_EQUAL( 13, dg.reachable_word_graph_nodes(nodes) );
    dg.tie_subword_suffixes(nodes);
    CPPUNIT_ASSERT_EQUAL( 12, dg.reachable_word_graph_nodes(nodes) );
}


// Test expanding the subwords to triphones
void graphtest::GraphTest3(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    CPPUNIT_ASSERT_EQUAL( 182, (int)nodes.size() );
    CPPUNIT_ASSERT_EQUAL( 182, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 12, dg.num_subword_states(nodes) );

    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
}


// Test tying state chain prefixes
void graphtest::GraphTest4(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    CPPUNIT_ASSERT_EQUAL( 182, (int)dg.reachable_graph_nodes(nodes) );
    dg.tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT_EQUAL( 139, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
}


// Verify adding self transitions and transition probabilities to states
void graphtest::GraphTest5(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.tie_state_prefixes(nodes, false);
    dg.tie_state_suffixes(nodes);
    dg.prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );

    dg.add_hmm_self_transitions(nodes);
    //dg.set_hmm_transition_probs(nodes);
    //CPPUNIT_ASSERT( assert_transitions(dg, nodes, true) );
}


// Test pruning non-reachable nodes and reindexing nodes
void graphtest::GraphTest6(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    CPPUNIT_ASSERT_EQUAL( 182, (int)dg.reachable_graph_nodes(nodes) );
    dg.tie_state_prefixes(nodes, false);
    dg.tie_state_suffixes(nodes);
    CPPUNIT_ASSERT_EQUAL( 136, (int)dg.reachable_graph_nodes(nodes) );

    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 136, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 136, (int)nodes.size() );

    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
}


// Test pushing subword ids to the leftmost possible position
void graphtest::GraphTest7(void)
{
    DecoderGraph dg;
    //segname = "data/iter111_35000.train.segs2";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    CPPUNIT_ASSERT_EQUAL( 182, (int)dg.reachable_graph_nodes(nodes) );

    dg.tie_state_prefixes(nodes, false);
    dg.prune_unreachable_nodes(nodes);
    dg.tie_state_suffixes(nodes);
    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT_EQUAL( 136, (int)dg.reachable_graph_nodes(nodes) );

    dg.push_word_ids_left(nodes);
    dg.prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT_EQUAL( 136, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 136, (int)nodes.size() );

    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes, false) );
}


// Test some subword id push problem
void graphtest::GraphTest8(void)
{
    DecoderGraph dg;
    segname = "data/push_problem.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );

    dg.tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    dg.tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT_EQUAL( 4, dg.num_subword_states(nodes) );

    dg.push_word_ids_left(nodes);
    CPPUNIT_ASSERT_EQUAL( 4, dg.num_subword_states(nodes) );
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes, true) );

    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
}


// Test some pathological cases
void graphtest::GraphTest9(void)
{
    DecoderGraph dg;
    segname = "data/segs2.txt";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );

    dg.tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );

    dg.tie_state_suffixes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );

    dg.push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );

    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
}


// Test triphonization with lexicon
void graphtest::GraphTest10(void)
{
    DecoderGraph dg;
    segname = "data/segs3.txt";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );

    dg.tie_state_prefixes(nodes, false);
    dg.tie_state_suffixes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
}


// Test some state suffix tying problem
void graphtest::GraphTest11(void)
{
    DecoderGraph dg;
    segname = "data/suffix_problem.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );

    dg.tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    dg.tie_state_suffixes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
}


// Test cross-word network creation and connecting, very simple case
void graphtest::GraphTest12(void)
{
    DecoderGraph dg;
    segname = "data/cw_simpler.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 34, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);

    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 62, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// Test cross-word network creation and connecting, another simple case
void graphtest::GraphTest13(void)
{
    DecoderGraph dg;
    segname = "data/cw_simple.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 80, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);

    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 121, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// Test cross-word network creation and connecting, easy case
void graphtest::GraphTest14(void)
{
    DecoderGraph dg;
    segname = "data/segs.txt";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 182, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);

    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 338, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// Test cross-word network creation and connecting
// 2 phone words and other special cases
void graphtest::GraphTest15(void)
{
    DecoderGraph dg;
    segname = "data/segs2.txt";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 67, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);
    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 181, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// Test cross-word network creation and connecting
// Normal cases
// Tie prefixes and suffixes after connecting cw network
void graphtest::GraphTest16(void)
{
    DecoderGraph dg;
    segname = "data/cw_simple.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 80, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);

    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 121, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );

    dg.push_word_ids_right(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_right(dg, nodes));

    dg.tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT_EQUAL( 104, (int)dg.reachable_graph_nodes(nodes) );
    //CPPUNIT_ASSERT( assert_prefix_state_tying(dg, nodes) );
    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );

    dg.push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes));
    dg.tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT_EQUAL( 92, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// Test cross-word network creation and connecting
// Tie prefixes and suffixes after connecting cw network
// Some problem cases, 4 phone words segmented to 2 phones + 2 phones
void graphtest::GraphTest17(void)
{
    DecoderGraph dg;
    segname = "data/cw_problem.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 30, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);
    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 45, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// Test cross-word network creation and connecting
// Problem in tying prefixes
void graphtest::GraphTest18(void)
{
    DecoderGraph dg;
    segname = "data/prefix_tie_problem.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);
    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

    dg.push_word_ids_right(nodes);
    dg.tie_state_prefixes(nodes, false);

    dg.push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes));
    dg.tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// Verify that only true suffixes are tied
void graphtest::GraphTest19(void)
{
    DecoderGraph dg;
    segname = "data/subword_tie_only_real_suffix.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
}


// Problem in expanding subwords to states
void graphtest::GraphTest20(void)
{
    DecoderGraph dg;
    segname = "data/subword_tie_expand_problem.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
}


// Some issue tie + expand
void graphtest::GraphTest21(void)
{
    DecoderGraph dg;
    segname = "data/subword_tie_expand_problem_2.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
}


// Extra check for some prefix tying special case
void graphtest::GraphTest22(void)
{
    DecoderGraph dg;
    segname = "data/prefix_tie_problem_2.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);
    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );

    dg.push_word_ids_right(nodes);
    dg.tie_state_prefixes(nodes, false);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// Some problem with duplicate word ids
void graphtest::GraphTest23(void)
{
    DecoderGraph dg;
    segname = "data/duplicate2.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);
    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    //CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );

    dg.push_word_ids_right(nodes);
    dg.tie_state_prefixes(nodes, false);

    dg.push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes));
    dg.tie_state_suffixes(nodes);
    dg.tie_state_prefixes(nodes);
    dg.tie_word_id_prefixes(nodes);
    dg.push_word_ids_left(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// Some problem with duplicate word ids
void graphtest::GraphTest24(void)
{
    DecoderGraph dg;
    segname = "data/au.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);
    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );

    dg.push_word_ids_right(nodes);
    dg.tie_state_prefixes(nodes, false);

    dg.push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes));
    dg.tie_state_suffixes(nodes);
    dg.tie_state_prefixes(nodes);
    dg.push_word_ids_left(nodes);
    dg.tie_word_id_prefixes(nodes);
    dg.push_word_ids_left(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// Test cross-word network creation and connecting
// More like a real scenario with 500 words with all tying etc.
// Print out some numbers
void graphtest::GraphTest25(void)
{
    DecoderGraph dg;
    segname = "data/500.segs";
    read_fixtures(dg);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    dg.expand_subword_nodes(swnodes, nodes);
    dg.prune_unreachable_nodes(nodes);
    cerr << endl << "real-like scenario with 500 words:" << endl;
    cerr << "initial expansion, number of nodes: " << dg.reachable_graph_nodes(nodes) << endl;

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network(cw_nodes, fanout, fanin);
    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);
    cerr << "crossword network added, number of nodes: " << dg.reachable_graph_nodes(nodes) << endl;

    dg.push_word_ids_right(nodes);
    dg.tie_state_prefixes(nodes, false);
    cerr << "prefixes tied, number of nodes: " << dg.reachable_graph_nodes(nodes) << endl;

    dg.push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes));
    dg.tie_state_suffixes(nodes);
    cerr << "suffixes tied, number of nodes: " << dg.reachable_graph_nodes(nodes) << endl;

    cerr << "asserting no double arcs" << endl;
    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    cerr << "asserting all words in the graph" << endl;
    CPPUNIT_ASSERT( assert_words(dg, nodes, true) );
    cerr << "asserting only correct words in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes) );
    cerr << "asserting all word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
    cerr << "asserting only correct word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes) );
}


// ofstream origoutf("cw_simple.dot");
// dg.print_dot_digraph(nodes, origoutf);
// origoutf.close();
