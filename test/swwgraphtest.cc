#include <boost/test/unit_test.hpp>

#include "ConstrainedSWGraph.hh"

using namespace std;


map<string, vector<string> > word_segs;


void read_fixtures(SWWGraph &swwg,
                   string segfname = "data/segs.txt")
{
    swwg.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    swwg.read_noway_lexicon("data/lex");
    word_segs.clear();
    swwg.read_word_segmentations(segfname, word_segs);
}


// Test tying state chain prefixes
BOOST_AUTO_TEST_CASE(SWWGraphTest1)
{
    SWWGraph swwg;
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    BOOST_CHECK_EQUAL( 145, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    BOOST_CHECK_EQUAL( 145, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
}


// Verify adding self transitions and transition probabilities to states
BOOST_AUTO_TEST_CASE(SWWGraphTest2)
{
    SWWGraph swwg;
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );

    swwg.add_hmm_self_transitions();
    //dg.set_hmm_transition_probs(swwg.m_nodes);
    //BOOST_CHECK( assert_transitions(dg, nodes, true) );
}


// Test pruning non-reachable nodes and reindexing nodes
BOOST_AUTO_TEST_CASE(SWWGraphTest3)
{
    SWWGraph swwg;
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    BOOST_CHECK_EQUAL( 145, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    BOOST_CHECK_EQUAL( 137, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );

    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);
    BOOST_CHECK_EQUAL( 137, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    BOOST_CHECK_EQUAL( 137, (int)swwg.m_nodes.size() );

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
}


// Test pushing subword ids to the leftmost possible position
BOOST_AUTO_TEST_CASE(SWWGraphTest4)
{
    SWWGraph swwg;
    read_fixtures(swwg);

    swwg.create_graph(word_segs, true, false);

    BOOST_CHECK_EQUAL( 145, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);
    BOOST_CHECK_EQUAL( 137, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );

    DecoderGraph::push_word_ids_left(swwg.m_nodes);
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);

    BOOST_CHECK_EQUAL( 137, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    BOOST_CHECK_EQUAL( 137, (int)swwg.m_nodes.size() );

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( DecoderGraph::assert_subword_ids_left(swwg.m_nodes) );
}


// Test some subword id push problem
BOOST_AUTO_TEST_CASE(SWWGraphTest5)
{
    SWWGraph swwg;
    string segname = "data/push_problem.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, true, false);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    DecoderGraph::prune_unreachable_nodes(swwg.m_nodes);
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK_EQUAL( 5, DecoderGraph::num_subword_states(swwg.m_nodes) );

    DecoderGraph::push_word_ids_left(swwg.m_nodes);
    BOOST_CHECK_EQUAL( 5, DecoderGraph::num_subword_states(swwg.m_nodes) );
    BOOST_CHECK( DecoderGraph::assert_subword_ids_left(swwg.m_nodes) );

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
}


// Test some pathological cases
BOOST_AUTO_TEST_CASE(SWWGraphTest6)
{
    SWWGraph swwg;
    string segname = "data/segs2.txt";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, true, false);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::push_word_ids_left(swwg.m_nodes);
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );

    BOOST_CHECK( DecoderGraph::assert_subword_ids_left(swwg.m_nodes) );
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
}


// Test triphonization with lexicon
BOOST_AUTO_TEST_CASE(SWWGraphTest7)
{
    SWWGraph swwg;
    string segname = "data/segs3.txt";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, true, false);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
}


// Test some state suffix tying problem
BOOST_AUTO_TEST_CASE(SWWGraphTest8)
{
    SWWGraph swwg;
    string segname = "data/suffix_problem.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, true, false);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
}


// Test cross-word network creation and connecting, very simple case
BOOST_AUTO_TEST_CASE(SWWGraphTest9)
{
    SWWGraph swwg;
    string segname = "data/cw_simpler.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, false, true);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting, another simple case
BOOST_AUTO_TEST_CASE(SWWGraphTest10)
{
    SWWGraph swwg;
    string segname = "data/cw_simple.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, false, true);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting, easy case
BOOST_AUTO_TEST_CASE(SWWGraphTest11)
{
    SWWGraph swwg;
    string segname = "data/segs.txt";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, false, true);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// 2 phone words and other special cases
BOOST_AUTO_TEST_CASE(SWWGraphTest12)
{
    SWWGraph swwg;
    string segname = "data/segs2.txt";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, false, true);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// Normal cases
// Tie prefixes and suffixes after connecting cw network
BOOST_AUTO_TEST_CASE(SWWGraphTest13)
{
    SWWGraph swwg;
    string segname = "data/cw_simple.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, false, true);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );

    DecoderGraph::push_word_ids_right(swwg.m_nodes);
    BOOST_CHECK( DecoderGraph::assert_subword_ids_right(swwg.m_nodes));

    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);

    DecoderGraph::push_word_ids_left(swwg.m_nodes);
    BOOST_CHECK( DecoderGraph::assert_subword_ids_left(swwg.m_nodes));
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);

    BOOST_CHECK_EQUAL( 96, (int)DecoderGraph::reachable_graph_nodes(swwg.m_nodes) );
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// Tie prefixes and suffixes after connecting cw network
// Some problem cases, 4 phone words segmented to 2 phones + 2 phones
BOOST_AUTO_TEST_CASE(SWWGraphTest14)
{
    SWWGraph swwg;
    string segname = "data/cw_problem.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, false, true);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// Problem in tying prefixes
BOOST_AUTO_TEST_CASE(SWWGraphTest15)
{
    SWWGraph swwg;
    string segname = "data/prefix_tie_problem.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, false, true);

    DecoderGraph::push_word_ids_right(swwg.m_nodes);
    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);

    DecoderGraph::push_word_ids_left(swwg.m_nodes);
    BOOST_CHECK( DecoderGraph::assert_subword_ids_left(swwg.m_nodes));
    DecoderGraph::tie_state_suffixes(swwg.m_nodes);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Verify that only true suffixes are tied | NOTE: for subword-triphone-phone builder
BOOST_AUTO_TEST_CASE(SWWGraphTest16)
{
    SWWGraph swwg;
    string segname = "data/subword_tie_only_real_suffix.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, true, false);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
}


// Problem in expanding subwords to states | NOTE: for subword-triphone-phone builder
BOOST_AUTO_TEST_CASE(SWWGraphTest17)
{
    SWWGraph swwg;
    string segname = "data/subword_tie_expand_problem.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, true, false);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
}


// Some issue tie + expand | NOTE: for subword-triphone-phone builder
BOOST_AUTO_TEST_CASE(SWWGraphTest18)
{
    SWWGraph swwg;
    string segname = "data/subword_tie_expand_problem_2.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, true, false);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
}


// Extra check for some prefix tying special case
BOOST_AUTO_TEST_CASE(SWWGraphTest19)
{
    SWWGraph swwg;
    string segname = "data/prefix_tie_problem_2.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, false, true);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );

    DecoderGraph::push_word_ids_right(swwg.m_nodes);
    DecoderGraph::tie_state_prefixes(swwg.m_nodes, false);

    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Some problem with duplicate word ids
BOOST_AUTO_TEST_CASE(SWWGraphTest20)
{
    SWWGraph swwg;
    string segname = "data/duplicate2.segs";
    read_fixtures(swwg, segname);

    swwg.create_graph(word_segs, false, true);

    //BOOST_CHECK( swwg.assert_no_duplicate_word_ids() );
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );

    swwg.tie_graph(false, false);

    BOOST_CHECK( swwg.assert_no_duplicate_word_ids() );
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Some problem with duplicate word ids
BOOST_AUTO_TEST_CASE(SWWGraphTest21)
{
    SWWGraph swwg;
    string segname = "data/au.segs";
    read_fixtures(swwg, segname);
    cerr << endl;

    swwg.create_graph(word_segs, false, true);
    swwg.tie_graph(false, false);

    BOOST_CHECK( swwg.assert_no_duplicate_word_ids() );
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// More like a real scenario with 500 words with all tying etc.
BOOST_AUTO_TEST_CASE(SWWGraphTest22)
{
    SWWGraph swwg;
    string segname = "data/500.segs";
    read_fixtures(swwg, segname);
    cerr << endl;

    swwg.create_graph(word_segs, false, true);
    swwg.tie_graph(false, false);

    BOOST_CHECK( swwg.assert_no_duplicate_word_ids() );
    cerr << "asserting all words in the graph" << endl;
    BOOST_CHECK( swwg.assert_words(word_segs) );
    cerr << "asserting only correct words in the graph" << endl;
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    cerr << "asserting all word pairs in the graph" << endl;
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    cerr << "asserting only correct word pairs in the graph" << endl;
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test a tying problem
BOOST_AUTO_TEST_CASE(SWWGraphTest23)
{
    SWWGraph swwg;
    string segname = "data/auto.segs";
    read_fixtures(swwg, segname);
    cerr << endl;

    swwg.create_graph(word_segs, false, true);
    swwg.tie_graph(false, false);

    BOOST_CHECK( swwg.assert_no_duplicate_word_ids() );
    BOOST_CHECK( swwg.assert_words(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// Test cross-word network creation and connecting
// More like a real scenario with 500 words with all tying etc.
// Print out some numbers
BOOST_AUTO_TEST_CASE(SWWGraphTest24)
{
    SWWGraph swwg;
    string segname = "data/bg.segs";
    read_fixtures(swwg, segname);
    cerr << endl;

    swwg.create_graph(word_segs, false, true);
    swwg.tie_graph(false, false);

    cerr << "asserting all words in the graph" << endl;
    BOOST_CHECK( swwg.assert_words(word_segs) );
    cerr << "asserting only correct words in the graph" << endl;
    BOOST_CHECK( swwg.assert_only_segmented_words(word_segs) );
    cerr << "asserting all word pairs in the graph" << endl;
    BOOST_CHECK( swwg.assert_word_pairs(word_segs) );
    cerr << "asserting only correct word pairs in the graph" << endl;
    BOOST_CHECK( swwg.assert_only_segmented_cw_word_pairs(word_segs) );
}


// ofstream origoutf("cw_simple.dot");
// print_dot_digraph(dg, nodes, origoutf);
// origoutf.close();
