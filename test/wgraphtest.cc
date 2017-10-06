#include <boost/test/unit_test.hpp>

#include "WordGraph.hh"

using namespace std;


void read_fixtures(WordGraph &wg,
                   string lexfname="data/1k.words.lex")
{
    wg.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    wg.read_noway_lexicon(lexfname);
}


// Test word graph construction with 1000 words
// No one phone words
BOOST_AUTO_TEST_CASE(WordGraphTest1)
{

    WordGraph wg;
    read_fixtures(wg);

    set<string> words;
    wg.read_words("data/1k.words.txt", words);

    cerr << endl;
    wg.create_graph(words, false);
    wg.tie_graph(false);
    cerr << "Number of lm id nodes: " << DecoderGraph::num_subword_states(wg.m_nodes) << endl;

    BOOST_CHECK( wg.assert_words(words) );
    BOOST_CHECK( wg.assert_only_words(words) );
    BOOST_CHECK( wg.assert_word_pairs(words, 20000) );
    BOOST_CHECK( wg.assert_only_cw_word_pairs(words) );
}


// Test word graph construction with 1000+ words
// Includes one phone words
BOOST_AUTO_TEST_CASE(WordGraphTest2)
{
    WordGraph wg;
    string lexname = "data/500.words.1pwords.lex";
    read_fixtures(wg, lexname);

    set<string> words;
    wg.read_words("data/500.words.1pwords.txt", words);

    cerr << endl;
    wg.create_graph(words, false);
    wg.tie_graph(false);
    cerr << "Number of lm id nodes: " << DecoderGraph::num_subword_states(wg.m_nodes) << endl;

    BOOST_CHECK_EQUAL( 524, DecoderGraph::num_subword_states(wg.m_nodes) );
    BOOST_CHECK( wg.assert_words(words) );
    BOOST_CHECK( wg.assert_only_words(words) );
    BOOST_CHECK( wg.assert_word_pairs(words, 20000) );
    BOOST_CHECK( wg.assert_only_cw_word_pairs(words) );
}

