
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


void wgraphtest::read_fixtures(WordGraph &wg)
{
    wg.read_phone_model(amname + ".ph");
    wg.read_noway_lexicon(lexname);
}


// Test word graph construction with 1000 words
// No one phone words
void wgraphtest::WordGraphTest1(void)
{

    WordGraph wg;
    read_fixtures(wg);

    set<string> words;
    wg.read_words("data/1k.words.txt", words);

    cerr << endl;
    wg.create_graph(words, false);
    wg.tie_graph(false);
    cerr << "Number of lm id nodes: " << DecoderGraph::num_subword_states(wg.m_nodes) << endl;

    CPPUNIT_ASSERT( wg.assert_words(words) );
    CPPUNIT_ASSERT( wg.assert_only_words(words) );
    CPPUNIT_ASSERT( wg.assert_word_pairs(words, 20000) );
    CPPUNIT_ASSERT( wg.assert_only_cw_word_pairs(words) );
}


// Test word graph construction with 1000+ words
// Includes one phone words
void wgraphtest::WordGraphTest2(void)
{
    WordGraph wg;
    lexname = "data/500.words.1pwords.lex";
    read_fixtures(wg);

    set<string> words;
    wg.read_words("data/500.words.1pwords.txt", words);

    cerr << endl;
    wg.create_graph(words, false);
    wg.tie_graph(false);
    cerr << "Number of lm id nodes: " << DecoderGraph::num_subword_states(wg.m_nodes) << endl;

    CPPUNIT_ASSERT_EQUAL( 524, DecoderGraph::num_subword_states(wg.m_nodes) );
    CPPUNIT_ASSERT( wg.assert_words(words) );
    CPPUNIT_ASSERT( wg.assert_only_words(words) );
    CPPUNIT_ASSERT( wg.assert_word_pairs(words, 20000) );
    CPPUNIT_ASSERT( wg.assert_only_cw_word_pairs(words) );
}

