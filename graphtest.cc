#include <string>
#include <iostream>
#include <fstream>

#include "graphtest.hh"
#include "gutils.hh"
#include "GraphBuilder1.hh"
#include "GraphBuilder2.hh"

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
    read_word_segmentations(dg, segname, word_segs);
}


// Verify that models are correctly loaded
// Test constructing the initial word graph on subword level
void graphtest::GraphTest1(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    CPPUNIT_ASSERT_EQUAL( 35003, (int)dg.m_subwords.size() );
    CPPUNIT_ASSERT_EQUAL( 35003, (int)dg.m_subword_map.size() ); // ?
    CPPUNIT_ASSERT_EQUAL( 13252, (int)dg.m_hmms.size() );
    CPPUNIT_ASSERT_EQUAL( 13252, (int)dg.m_hmm_map.size() );
    CPPUNIT_ASSERT_EQUAL( 1170, (int)dg.m_hmm_states.size() );
    CPPUNIT_ASSERT_EQUAL( 11, (int)word_segs.size() );

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    CPPUNIT_ASSERT_EQUAL( 13, reachable_word_graph_nodes(swnodes) );
}


// Test tying word suffixes
void graphtest::GraphTest2(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> nodes;
    graphbuilder1::create_word_graph(dg, nodes, word_segs);
    CPPUNIT_ASSERT_EQUAL( 13, reachable_word_graph_nodes(nodes) );
    tie_subword_suffixes(nodes);
    CPPUNIT_ASSERT_EQUAL( 12, reachable_word_graph_nodes(nodes) );
}


// Test expanding the subwords to triphones
void graphtest::GraphTest3(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    CPPUNIT_ASSERT_EQUAL( 182, (int)nodes.size() );
    CPPUNIT_ASSERT_EQUAL( 182, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 12, num_subword_states(nodes) );

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test tying state chain prefixes
void graphtest::GraphTest4(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    CPPUNIT_ASSERT_EQUAL( 182, (int)reachable_graph_nodes(nodes) );
    tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT_EQUAL( 139, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Verify adding self transitions and transition probabilities to states
void graphtest::GraphTest5(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    tie_state_prefixes(nodes, false);
    tie_state_suffixes(nodes);
    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    add_hmm_self_transitions(nodes);
    //dg.set_hmm_transition_probs(nodes);
    //CPPUNIT_ASSERT( assert_transitions(dg, nodes, true) );
}


// Test pruning non-reachable nodes and reindexing nodes
void graphtest::GraphTest6(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    CPPUNIT_ASSERT_EQUAL( 182, (int)reachable_graph_nodes(nodes) );
    tie_state_prefixes(nodes, false);
    tie_state_suffixes(nodes);
    CPPUNIT_ASSERT_EQUAL( 136, (int)reachable_graph_nodes(nodes) );

    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 136, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 136, (int)nodes.size() );

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test pushing subword ids to the leftmost possible position
void graphtest::GraphTest7(void)
{
    DecoderGraph dg;
    //segname = "data/iter111_35000.train.segs2";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    CPPUNIT_ASSERT_EQUAL( 182, (int)reachable_graph_nodes(nodes) );

    tie_state_prefixes(nodes, false);
    prune_unreachable_nodes(nodes);
    tie_state_suffixes(nodes);
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT_EQUAL( 136, (int)reachable_graph_nodes(nodes) );

    push_word_ids_left(nodes);
    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT_EQUAL( 136, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 136, (int)nodes.size() );

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes, false) );
}


// Test some subword id push problem
void graphtest::GraphTest8(void)
{
    DecoderGraph dg;
    segname = "data/push_problem.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT_EQUAL( 4, num_subword_states(nodes) );

    push_word_ids_left(nodes);
    CPPUNIT_ASSERT_EQUAL( 4, num_subword_states(nodes) );
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes, true) );

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test some pathological cases
void graphtest::GraphTest9(void)
{
    DecoderGraph dg;
    segname = "data/segs2.txt";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    tie_state_suffixes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test triphonization with lexicon
void graphtest::GraphTest10(void)
{
    DecoderGraph dg;
    segname = "data/segs3.txt";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    tie_state_prefixes(nodes, false);
    tie_state_suffixes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test some state suffix tying problem
void graphtest::GraphTest11(void)
{
    DecoderGraph dg;
    segname = "data/suffix_problem.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    tie_state_suffixes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting, very simple case
void graphtest::GraphTest12(void)
{
    DecoderGraph dg;
    segname = "data/cw_simpler.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 34, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 66, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting, another simple case
void graphtest::GraphTest13(void)
{
    DecoderGraph dg;
    segname = "data/cw_simple.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 80, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 127, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting, easy case
void graphtest::GraphTest14(void)
{
    DecoderGraph dg;
    segname = "data/segs.txt";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 182, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 365, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// 2 phone words and other special cases
void graphtest::GraphTest15(void)
{
    DecoderGraph dg;
    segname = "data/segs2.txt";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 67, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 201, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// Normal cases
// Tie prefixes and suffixes after connecting cw network
void graphtest::GraphTest16(void)
{
    DecoderGraph dg;
    segname = "data/cw_simple.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 80, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 127, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );

    push_word_ids_right(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_right(dg, nodes));

    tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT_EQUAL( 110, (int)reachable_graph_nodes(nodes) );
    //CPPUNIT_ASSERT( assert_prefix_state_tying(dg, nodes) );
    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );

    push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes));
    tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT_EQUAL( 98, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// Tie prefixes and suffixes after connecting cw network
// Some problem cases, 4 phone words segmented to 2 phones + 2 phones
void graphtest::GraphTest17(void)
{
    DecoderGraph dg;
    segname = "data/cw_problem.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 30, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT_EQUAL( 47, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// Problem in tying prefixes
void graphtest::GraphTest18(void)
{
    DecoderGraph dg;
    segname = "data/prefix_tie_problem.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);

    push_word_ids_right(nodes);
    tie_state_prefixes(nodes, false);

    push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes));
    tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Verify that only true suffixes are tied
void graphtest::GraphTest19(void)
{
    DecoderGraph dg;
    segname = "data/subword_tie_only_real_suffix.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Problem in expanding subwords to states
void graphtest::GraphTest20(void)
{
    DecoderGraph dg;
    segname = "data/subword_tie_expand_problem.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Some issue tie + expand
void graphtest::GraphTest21(void)
{
    DecoderGraph dg;
    segname = "data/subword_tie_expand_problem_2.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Extra check for some prefix tying special case
void graphtest::GraphTest22(void)
{
    DecoderGraph dg;
    segname = "data/prefix_tie_problem_2.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );

    push_word_ids_right(nodes);
    tie_state_prefixes(nodes, false);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Some problem with duplicate word ids
void graphtest::GraphTest23(void)
{
    DecoderGraph dg;
    segname = "data/duplicate2.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    //CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );

    push_word_ids_right(nodes);
    tie_state_prefixes(nodes, false);

    push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes));
    tie_state_suffixes(nodes);
    tie_state_prefixes(nodes);
    tie_word_id_prefixes(nodes);
    push_word_ids_left(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Some problem with duplicate word ids
void graphtest::GraphTest24(void)
{
    DecoderGraph dg;
    segname = "data/au.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);

    push_word_ids_left(nodes);
    tie_state_prefixes(nodes);
    tie_word_id_prefixes(nodes);
    push_word_ids_right(nodes);
    tie_state_prefixes(nodes);

    push_word_ids_right(nodes);
    tie_state_suffixes(nodes);
    tie_word_id_suffixes(nodes);
    push_word_ids_left(nodes);
    tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// More like a real scenario with 500 words with all tying etc.
// Print out some numbers
void graphtest::GraphTest25(void)
{
    DecoderGraph dg;
    segname = "data/500.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);
    cerr << endl << "real-like scenario with 500 words:" << endl;
    cerr << "initial expansion, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);
    cerr << "crossword network added, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    push_word_ids_left(nodes);
    tie_state_prefixes(nodes);
    tie_word_id_prefixes(nodes);
    push_word_ids_right(nodes);
    tie_state_prefixes(nodes);
    cerr << "prefixes tied, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    push_word_ids_right(nodes);
    tie_state_suffixes(nodes);
    tie_word_id_suffixes(nodes);
    push_word_ids_left(nodes);
    tie_state_suffixes(nodes);
    cerr << "suffixes tied, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    cerr << "asserting no double arcs" << endl;
    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    cerr << "asserting all words in the graph" << endl;
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    cerr << "asserting only correct words in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    cerr << "asserting all word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    cerr << "asserting only correct word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// More like a real scenario with 500 words with all tying etc.
// Alternative graph construction
void graphtest::GraphTest26(void)
{
    DecoderGraph dg;
    segname = "data/500.segs";
    read_fixtures(dg);

    /*
    vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
    map<string, vector<string> > triphonized_words;
    triphonize_all_words(dg, triphonized_words);
    for (auto wit = triphonized_words.begin(); wit != triphonized_words.end(); ++wit)
        dg.add_word(triphone_nodes, wit->first, wit->second);
    */

    vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit) {
        vector<DecoderGraph::TriphoneNode> word_triphones;
        triphonize(dg, wit->second, word_triphones);
        add_triphones(triphone_nodes, word_triphones);
    }

    vector<DecoderGraph::Node> nodes(2);
    triphones_to_states(dg, triphone_nodes, nodes);
    triphone_nodes.clear();

    prune_unreachable_nodes(nodes);
    cerr << endl << "real-like scenario with 500 words:" << endl;
    cerr << "initial expansion, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder2::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder2::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin, false);
    connect_end_to_start_node(nodes);
    cerr << "crossword network added, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    push_word_ids_left(nodes);
    tie_state_prefixes(nodes);
    tie_word_id_prefixes(nodes);
    push_word_ids_right(nodes);
    tie_state_prefixes(nodes);
    cerr << "prefixes tied, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    push_word_ids_right(nodes);
    tie_state_suffixes(nodes);
    tie_word_id_suffixes(nodes);
    push_word_ids_left(nodes);
    tie_state_suffixes(nodes);
    cerr << "suffixes tied, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    cerr << "asserting no double arcs" << endl;
    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    cerr << "asserting all words in the graph" << endl;
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    cerr << "asserting only correct words in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    cerr << "asserting all word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    cerr << "asserting only correct word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test the tying problem with the alternative graph construction
void graphtest::GraphTest27(void)
{
    DecoderGraph dg;
    segname = "data/auto.segs";
    read_fixtures(dg);

    /*
    vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
    map<string, vector<string> > triphonized_words;
    triphonize_all_words(dg, triphonized_words);
    for (auto wit = triphonized_words.begin(); wit != triphonized_words.end(); ++wit)
        dg.add_word(triphone_nodes, wit->first, wit->second);
    */

    vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit) {
        vector<DecoderGraph::TriphoneNode> word_triphones;
        triphonize(dg, wit->second, word_triphones);
        add_triphones(triphone_nodes, word_triphones);
    }

    vector<DecoderGraph::Node> nodes(2);
    triphones_to_states(dg, triphone_nodes, nodes);
    triphone_nodes.clear();
    prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder2::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder2::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin, false);
    connect_end_to_start_node(nodes);

    tie_state_prefixes(nodes);
    tie_word_id_prefixes(nodes);
    push_word_ids_right(nodes);
    tie_state_prefixes(nodes);

    push_word_ids_right(nodes);
    tie_state_suffixes(nodes);
    tie_word_id_suffixes(nodes);
    push_word_ids_left(nodes);
    tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// More like a real scenario with 500 words with all tying etc.
// Print out some numbers
void graphtest::GraphTest28(void)
{
    DecoderGraph dg;
    segname = "data/bg.segs";
    read_fixtures(dg);

    vector<graphbuilder1::SubwordNode> swnodes;
    graphbuilder1::create_word_graph(dg, swnodes, word_segs);
    tie_subword_suffixes(swnodes);

    vector<DecoderGraph::Node> nodes;
    expand_subword_nodes(dg, swnodes, nodes);
    prune_unreachable_nodes(nodes);
    cerr << "initial expansion, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    graphbuilder1::create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin);
    graphbuilder1::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);
    cerr << "crossword network added, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    push_word_ids_left(nodes);
    tie_state_prefixes(nodes);
    tie_word_id_prefixes(nodes);
    push_word_ids_right(nodes);
    tie_state_prefixes(nodes);
    cerr << "prefixes tied, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    push_word_ids_right(nodes);
    tie_state_suffixes(nodes);
    tie_word_id_suffixes(nodes);
    push_word_ids_left(nodes);
    tie_state_suffixes(nodes);
    cerr << "suffixes tied, number of nodes: " << reachable_graph_nodes(nodes) << endl;

    cerr << "asserting no double arcs" << endl;
    CPPUNIT_ASSERT( assert_no_double_arcs(nodes) );
    cerr << "asserting all words in the graph" << endl;
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, true) );
    cerr << "asserting only correct words in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    cerr << "asserting all word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    cerr << "asserting only correct word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// ofstream origoutf("cw_simple.dot");
// dg.print_dot_digraph(nodes, origoutf);
// origoutf.close();
