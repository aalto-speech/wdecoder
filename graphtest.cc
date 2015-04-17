#include "graphtest.hh"

using namespace std;


CPPUNIT_TEST_SUITE_REGISTRATION (swwgraphtest);


void swwgraphtest::setUp (void)
{
    amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
    lexname = string("data/lex");
    segname = string("data/segs.txt");
}


void swwgraphtest::tearDown (void)
{
}


void swwgraphtest::read_fixtures(SWWGraph &swwg)
{
    swwg.read_phone_model(amname + ".ph");
    swwg.read_noway_lexicon(lexname);
    word_segs.clear();
    swwg.read_word_segmentations(segname, word_segs);
}


// Test tying state chain prefixes
void swwgraphtest::SWWGraphTest1(void)
{
    SWWGraph swwg;
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    CPPUNIT_ASSERT_EQUAL( 145, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    CPPUNIT_ASSERT_EQUAL( 145, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
}


// Verify adding self transitions and transition probabilities to states
void swwgraphtest::SWWGraphTest2(void)
{
    SWWGraph swwg;
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );

    swwg.add_hmm_self_transitions();
    //dg.set_hmm_transition_probs(swwg.m_nodes);
    //CPPUNIT_ASSERT( assert_transitions(dg, nodes, true) );
}


// Test pruning non-reachable nodes and reindexing nodes
void swwgraphtest::SWWGraphTest3(void)
{
    SWWGraph swwg;
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    CPPUNIT_ASSERT_EQUAL( 145, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    CPPUNIT_ASSERT_EQUAL( 137, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );

    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);
    CPPUNIT_ASSERT_EQUAL( 137, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    CPPUNIT_ASSERT_EQUAL( 137, (int)swwg.m_nodes.size() );

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
}


// Test pushing subword ids to the leftmost possible position
void swwgraphtest::SWWGraphTest4(void)
{
    SWWGraph swwg;
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    CPPUNIT_ASSERT_EQUAL( 145, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);
    CPPUNIT_ASSERT_EQUAL( 137, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );

    DecoderGraph::push_word_ids_left(swwg.m_nodes);
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);

    CPPUNIT_ASSERT_EQUAL( 137, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    CPPUNIT_ASSERT_EQUAL( 137, (int)swwg.m_nodes.size() );

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( DecoderGraph::assert_subword_ids_left(swwg.m_nodes) );
}


// Test some subword id push problem
void swwgraphtest::SWWGraphTest5(void)
{
    SWWGraph swwg;
    segname = "data/push_problem.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT_EQUAL( 5, DecoderGraph::num_subword_states(swwg.m_nodes) );

    DecoderGraph::push_word_ids_left(swwg.m_nodes);
    CPPUNIT_ASSERT_EQUAL( 5, DecoderGraph::num_subword_states(swwg.m_nodes) );
    CPPUNIT_ASSERT( DecoderGraph::assert_subword_ids_left(swwg.m_nodes) );

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
}


// Test some pathological cases
void swwgraphtest::SWWGraphTest6(void)
{
    SWWGraph swwg;
    segname = "data/segs2.txt";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::push_word_ids_left(swwg.m_nodes);
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );

    CPPUNIT_ASSERT( DecoderGraph::assert_subword_ids_left(swwg.m_nodes) );
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
}


// Test triphonization with lexicon
void swwgraphtest::SWWGraphTest7(void)
{
    SWWGraph swwg;
    segname = "data/segs3.txt";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
}


// Test some state suffix tying problem
void swwgraphtest::SWWGraphTest8(void)
{
    SWWGraph swwg;
    segname = "data/suffix_problem.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
}


// Test cross-word network creation and connecting, very simple case
void swwgraphtest::SWWGraphTest9(void)
{
    SWWGraph swwg;
    segname = "data/cw_simpler.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, false, true);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting, another simple case
void swwgraphtest::SWWGraphTest10(void)
{
    SWWGraph swwg;
    segname = "data/cw_simple.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, false, true);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting, easy case
void swwgraphtest::SWWGraphTest11(void)
{
    SWWGraph swwg;
    segname = "data/segs.txt";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, false, true);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// 2 phone words and other special cases
void swwgraphtest::SWWGraphTest12(void)
{
    SWWGraph swwg;
    segname = "data/segs2.txt";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, false, true);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// Normal cases
// Tie prefixes and suffixes after connecting cw network
void swwgraphtest::SWWGraphTest13(void)
{
    SWWGraph swwg;
    segname = "data/cw_simple.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, false, true);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );

    DecoderGraph::push_word_ids_right(swwg.m_nodes);
    CPPUNIT_ASSERT( DecoderGraph::assert_subword_ids_right(swwg.m_nodes));

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);

    DecoderGraph::push_word_ids_left(swwg.m_nodes);
    CPPUNIT_ASSERT( DecoderGraph::assert_subword_ids_left(swwg.m_nodes));
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);

    CPPUNIT_ASSERT_EQUAL( 96, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// Tie prefixes and suffixes after connecting cw network
// Some problem cases, 4 phone words segmented to 2 phones + 2 phones
void swwgraphtest::SWWGraphTest14(void)
{
    SWWGraph swwg;
    segname = "data/cw_problem.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, false, true);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// Problem in tying prefixes
void swwgraphtest::SWWGraphTest15(void)
{
    SWWGraph swwg;
    segname = "data/prefix_tie_problem.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, false, true);

    DecoderGraph::push_word_ids_right(swwg.m_nodes);
    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);

    DecoderGraph::push_word_ids_left(swwg.m_nodes);
    CPPUNIT_ASSERT( DecoderGraph::assert_subword_ids_left(swwg.m_nodes));
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Verify that only true suffixes are tied | NOTE: for subword-triphone-phone builder
void swwgraphtest::SWWGraphTest16(void)
{
    SWWGraph swwg;
    segname = "data/subword_tie_only_real_suffix.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
}


// Problem in expanding subwords to states | NOTE: for subword-triphone-phone builder
void swwgraphtest::SWWGraphTest17(void)
{
    SWWGraph swwg;
    segname = "data/subword_tie_expand_problem.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
}


// Some issue tie + expand | NOTE: for subword-triphone-phone builder
void swwgraphtest::SWWGraphTest18(void)
{
    SWWGraph swwg;
    segname = "data/subword_tie_expand_problem_2.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
}


// Extra check for some prefix tying special case
void swwgraphtest::SWWGraphTest19(void)
{
    SWWGraph swwg;
    segname = "data/prefix_tie_problem_2.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, false, true);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );

    DecoderGraph::push_word_ids_right(swwg.m_nodes);
    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);

    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Some problem with duplicate word ids
void swwgraphtest::SWWGraphTest20(void)
{
    SWWGraph swwg;
    segname = "data/duplicate2.segs";
    read_fixtures(swwg);

    swwg.create_graph(word_segs, false, true);

    //CPPUNIT_ASSERT( swwg.assert_no_duplicate_word_ids() );
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );

    swwg.tie_graph(false, false);

    CPPUNIT_ASSERT( swwg.assert_no_duplicate_word_ids() );
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Some problem with duplicate word ids
void swwgraphtest::SWWGraphTest21(void)
{
    SWWGraph swwg;
    segname = "data/au.segs";
    read_fixtures(swwg);
    cerr << endl;

    swwg.create_graph(word_segs, false, true);
    swwg.tie_graph(false, false);

    CPPUNIT_ASSERT( swwg.assert_no_duplicate_word_ids() );
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// More like a real scenario with 500 words with all tying etc.
void swwgraphtest::SWWGraphTest22(void)
{
    SWWGraph swwg;
    segname = "data/500.segs";
    read_fixtures(swwg);
    cerr << endl;

    swwg.create_graph(word_segs, false, true);
    swwg.tie_graph(false, false);

    CPPUNIT_ASSERT( swwg.assert_no_duplicate_word_ids() );
    cerr << "asserting all words in the graph" << endl;
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    cerr << "asserting only correct words in the graph" << endl;
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    cerr << "asserting all word pairs in the graph" << endl;
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    cerr << "asserting only correct word pairs in the graph" << endl;
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test a tying problem
void swwgraphtest::SWWGraphTest23(void)
{
    SWWGraph swwg;
    segname = "data/auto.segs";
    read_fixtures(swwg);
    cerr << endl;

    swwg.create_graph(word_segs, false, true);
    swwg.tie_graph(false, false);

    CPPUNIT_ASSERT( swwg.assert_no_duplicate_word_ids() );
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// More like a real scenario with 500 words with all tying etc.
// Print out some numbers
void swwgraphtest::SWWGraphTest24(void)
{
    SWWGraph swwg;
    segname = "data/bg.segs";
    read_fixtures(swwg);
    cerr << endl;

    swwg.create_graph(word_segs, false, true);
    swwg.tie_graph(false, false);

    cerr << "asserting all words in the graph" << endl;
    CPPUNIT_ASSERT( swwg.assert_words(word_segs) );
    cerr << "asserting only correct words in the graph" << endl;
    CPPUNIT_ASSERT( swwg.assert_only_segmented_words(word_segs) );
    cerr << "asserting all word pairs in the graph" << endl;
    CPPUNIT_ASSERT( swwg.assert_word_pairs(word_segs) );
    cerr << "asserting only correct word pairs in the graph" << endl;
    CPPUNIT_ASSERT( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// ofstream origoutf("cw_simple.dot");
// print_dot_digraph(dg, nodes, origoutf);
// origoutf.close();
