
#include "graphtest.hh"
#include "gutils.hh"
#include "GraphBuilder.hh"


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
    word_segs.clear();
    read_word_segmentations(dg, segname, word_segs);
}


// Test tying state chain prefixes
void graphtest::GraphTest1(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    CPPUNIT_ASSERT_EQUAL( 145, (int)reachable_graph_nodes(nodes) );
    tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT_EQUAL( 145, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Verify adding self transitions and transition probabilities to states
void graphtest::GraphTest2(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    tie_state_prefixes(nodes, false);
    tie_state_suffixes(nodes);
    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    add_hmm_self_transitions(nodes);
    //dg.set_hmm_transition_probs(nodes);
    //CPPUNIT_ASSERT( assert_transitions(dg, nodes, true) );
}


// Test pruning non-reachable nodes and reindexing nodes
void graphtest::GraphTest3(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    CPPUNIT_ASSERT_EQUAL( 145, (int)reachable_graph_nodes(nodes) );
    tie_state_prefixes(nodes, false);
    tie_state_suffixes(nodes);
    CPPUNIT_ASSERT_EQUAL( 137, (int)reachable_graph_nodes(nodes) );

    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 137, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 137, (int)nodes.size() );

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test pushing subword ids to the leftmost possible position
void graphtest::GraphTest4(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    CPPUNIT_ASSERT_EQUAL( 145, (int)reachable_graph_nodes(nodes) );

    tie_state_prefixes(nodes, false);
    prune_unreachable_nodes(nodes);
    tie_state_suffixes(nodes);
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 137, (int)reachable_graph_nodes(nodes) );

    push_word_ids_left(nodes);
    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT_EQUAL( 137, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 137, (int)nodes.size() );

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes) );
}


// Test some subword id push problem
void graphtest::GraphTest5(void)
{
    DecoderGraph dg;
    segname = "data/push_problem.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT_EQUAL( 5, num_subword_states(nodes) );

    push_word_ids_left(nodes);
    CPPUNIT_ASSERT_EQUAL( 5, num_subword_states(nodes) );
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes) );

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test some pathological cases
void graphtest::GraphTest6(void)
{
    DecoderGraph dg;
    segname = "data/segs2.txt";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    tie_state_suffixes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test triphonization with lexicon
void graphtest::GraphTest7(void)
{
    DecoderGraph dg;
    segname = "data/segs3.txt";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    tie_state_prefixes(nodes, false);
    tie_state_suffixes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test some state suffix tying problem
void graphtest::GraphTest8(void)
{
    DecoderGraph dg;
    segname = "data/suffix_problem.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );

    tie_state_prefixes(nodes, false);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    tie_state_suffixes(nodes);
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting, very simple case
void graphtest::GraphTest9(void)
{
    DecoderGraph dg;
    segname = "data/cw_simpler.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting, another simple case
void graphtest::GraphTest10(void)
{
    DecoderGraph dg;
    segname = "data/cw_simple.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting, easy case
void graphtest::GraphTest11(void)
{
    DecoderGraph dg;
    segname = "data/segs.txt";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// 2 phone words and other special cases
void graphtest::GraphTest12(void)
{
    DecoderGraph dg;
    segname = "data/segs2.txt";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// Normal cases
// Tie prefixes and suffixes after connecting cw network
void graphtest::GraphTest13(void)
{
    DecoderGraph dg;
    segname = "data/cw_simple.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );

    push_word_ids_right(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_right(dg, nodes));

    tie_state_prefixes(nodes, false);

    push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes));
    tie_state_suffixes(nodes);

    CPPUNIT_ASSERT_EQUAL( 96, (int)reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// Tie prefixes and suffixes after connecting cw network
// Some problem cases, 4 phone words segmented to 2 phones + 2 phones
void graphtest::GraphTest14(void)
{
    DecoderGraph dg;
    segname = "data/cw_problem.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// Problem in tying prefixes
void graphtest::GraphTest15(void)
{
    DecoderGraph dg;
    segname = "data/prefix_tie_problem.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

    push_word_ids_right(nodes);
    tie_state_prefixes(nodes, false);

    push_word_ids_left(nodes);
    CPPUNIT_ASSERT( assert_subword_ids_left(dg, nodes));
    tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Verify that only true suffixes are tied | NOTE: for subword-triphone-phone builder
void graphtest::GraphTest16(void)
{
    DecoderGraph dg;
    segname = "data/subword_tie_only_real_suffix.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Problem in expanding subwords to states | NOTE: for subword-triphone-phone builder
void graphtest::GraphTest17(void)
{
    DecoderGraph dg;
    segname = "data/subword_tie_expand_problem.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Some issue tie + expand | NOTE: for subword-triphone-phone builder
void graphtest::GraphTest18(void)
{
    DecoderGraph dg;
    segname = "data/subword_tie_expand_problem_2.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, true, false);

    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
}


// Extra check for some prefix tying special case
void graphtest::GraphTest19(void)
{
    DecoderGraph dg;
    segname = "data/prefix_tie_problem_2.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );

    push_word_ids_right(nodes);
    tie_state_prefixes(nodes, false);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Some problem with duplicate word ids
void graphtest::GraphTest20(void)
{
    DecoderGraph dg;
    segname = "data/duplicate2.segs";
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

    //CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
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

    CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Some problem with duplicate word ids
void graphtest::GraphTest21(void)
{
    DecoderGraph dg;
    segname = "data/au.segs";
    read_fixtures(dg);
    cerr << endl;

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

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

    CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// More like a real scenario with 500 words with all tying etc.
void graphtest::GraphTest22(void)
{
    DecoderGraph dg;
    segname = "data/500.segs";
    read_fixtures(dg);
    cerr << endl;

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

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

    CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    cerr << "asserting all words in the graph" << endl;
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    cerr << "asserting only correct words in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    cerr << "asserting all word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    cerr << "asserting only correct word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test a tying problem
void graphtest::GraphTest23(void)
{
    DecoderGraph dg;
    segname = "data/auto.segs";
    read_fixtures(dg);
    cerr << endl;

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

    tie_state_prefixes(nodes);
    tie_word_id_prefixes(nodes);
    push_word_ids_right(nodes);
    tie_state_prefixes(nodes);

    push_word_ids_right(nodes);
    tie_state_suffixes(nodes);
    tie_word_id_suffixes(nodes);
    push_word_ids_left(nodes);
    tie_state_suffixes(nodes);

    CPPUNIT_ASSERT( assert_no_duplicate_word_ids(dg, nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// Test cross-word network creation and connecting
// More like a real scenario with 500 words with all tying etc.
// Print out some numbers
void graphtest::GraphTest24(void)
{
    DecoderGraph dg;
    segname = "data/bg.segs";
    read_fixtures(dg);
    cerr << endl;

    vector<DecoderGraph::Node> nodes(2);
    graphbuilder::create_graph(dg, nodes, word_segs, false, true);

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

    cerr << "asserting all words in the graph" << endl;
    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    cerr << "asserting only correct words in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_words(dg, nodes, word_segs) );
    cerr << "asserting all word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs) );
    cerr << "asserting only correct word pairs in the graph" << endl;
    CPPUNIT_ASSERT( assert_only_segmented_cw_word_pairs(dg, nodes, word_segs) );
}


// ofstream origoutf("cw_simple.dot");
// print_dot_digraph(dg, nodes, origoutf);
// origoutf.close();
