
#include "wgraphtest.hh"
#include "gutils.hh"
#include "WordGraphBuilder.hh"


using namespace std;
using namespace gutils;


CPPUNIT_TEST_SUITE_REGISTRATION (wgraphtest);


void wgraphtest::setUp (void)
{
    amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
    lexname = string("data/1k.words.lex");
}


void wgraphtest::tearDown (void)
{
}


void wgraphtest::read_fixtures(DecoderGraph &dg)
{
    dg.read_phone_model(amname + ".ph");
    dg.read_noway_lexicon(lexname);
}


// Test word graph construction with 1000 words
// No one phone words
void wgraphtest::WordGraphTest1(void)
{

    DecoderGraph dg;
    read_fixtures(dg);

    set<string> words;
    read_words(dg, "data/1k.words.txt", words);

    vector<DecoderGraph::Node> nodes(2);
    cerr << endl;
    wordgraphbuilder::make_graph(dg, words, nodes);
    cerr << "Number of lm id nodes: " << num_subword_states(nodes) << endl;

    CPPUNIT_ASSERT( assert_words(dg, nodes, words) );
    CPPUNIT_ASSERT( assert_only_words(dg, nodes, words) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, words, 20000) );
    CPPUNIT_ASSERT( assert_only_cw_word_pairs(dg, nodes, words) );
}


// Test word graph construction with 1000+ words
// Includes one phone words
void wgraphtest::WordGraphTest2(void)
{
    DecoderGraph dg;
    lexname = "data/500.words.1pwords.lex";
    read_fixtures(dg);

    set<string> words;
    read_words(dg, "data/500.words.1pwords.txt", words);

    vector<DecoderGraph::Node> nodes(2);
    cerr << endl;
    wordgraphbuilder::make_graph(dg, words, nodes);

    CPPUNIT_ASSERT_EQUAL( 524, num_subword_states(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, words) );
    CPPUNIT_ASSERT( assert_only_words(dg, nodes, words) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, words) );
    CPPUNIT_ASSERT( assert_only_cw_word_pairs(dg, nodes, words) );
}

