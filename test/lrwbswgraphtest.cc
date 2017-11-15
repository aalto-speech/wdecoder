#include <boost/test/unit_test.hpp>

#include "LRWBSubwordGraph.hh"

using namespace std;

string lrwb_amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
set<string> lrwb_prefix_subwords;
set<string> lrwb_stem_subwords;
set<string> lrwb_suffix_subwords;
set<string> lrwb_word_subwords;

namespace LRWBGraphTestUtils {
void read_fixtures(LRWBSubwordGraph &swg, string lexfname) {
    swg.read_phone_model(lrwb_amname + ".ph");
    swg.read_lexicon(lexfname,
                     lrwb_prefix_subwords,
                     lrwb_stem_subwords,
                     lrwb_suffix_subwords,
                     lrwb_word_subwords);
}

void word_subwords(const set<string> &word_subwords,
                   map<string, vector<string> > &word_segs) {
    for (auto wswit = word_subwords.begin(); wswit != word_subwords.end(); ++wswit)
        word_segs[*wswit] = { *wswit };
}

void pre_suf_words(const set<string> &prefix_subwords,
                   const set<string> &suffix_subwords,
                   map<string, vector<string> > &word_segs) {
    for (auto prefit = prefix_subwords.begin(); prefit != prefix_subwords.end(); ++prefit) {
        for (auto sufit = suffix_subwords.begin(); sufit != suffix_subwords.end(); ++sufit) {
            vector<string> word_seg;
            word_seg.push_back(*prefit);
            word_seg.push_back(*sufit);
            string wrd = *prefit + *sufit;
            word_segs[wrd] = word_seg;
        }
    }
}

void pre_stem_suf_words(const set<string> &prefix_subwords,
                        const set<string> &stem_subwords,
                        const set<string> &suffix_subwords,
                        map<string, vector<string> > &word_segs) {
    for (auto prefit = prefix_subwords.begin(); prefit != prefix_subwords.end(); ++prefit) {
        for (auto stemit = stem_subwords.begin(); stemit != stem_subwords.end(); ++stemit) {
            for (auto sufit = suffix_subwords.begin(); sufit != suffix_subwords.end(); ++sufit) {
                vector<string> word_seg;
                word_seg.push_back(*prefit);
                word_seg.push_back(*stemit);
                word_seg.push_back(*sufit);
                string wrd = *prefit + *stemit + *sufit;
                word_segs[wrd] = word_seg;
            }
        }
    }
}
};

// Only complete word subwords
BOOST_AUTO_TEST_CASE(LRWBSubwordGraphTest1)
{
    LRWBSubwordGraph swg;
    LRWBGraphTestUtils::read_fixtures(swg, "data/lrwb_1.lex");

    swg.create_graph(lrwb_prefix_subwords,
                     lrwb_stem_subwords,
                     lrwb_suffix_subwords,
                     lrwb_word_subwords);

    BOOST_CHECK( swg.assert_words(lrwb_word_subwords) );
    BOOST_CHECK( swg.assert_only_words(lrwb_word_subwords) );
    BOOST_CHECK( swg.assert_word_pairs(lrwb_word_subwords, true, false) ); //short sil, wb symbol
}


// Prefix, suffix and complete word subwords
// No stems or one phone subwords
BOOST_AUTO_TEST_CASE(LRWBSubwordGraphTest2)
{
    cerr << endl;
    LRWBSubwordGraph swg;
    LRWBGraphTestUtils::read_fixtures(swg, "data/lrwb_2.lex");

    swg.create_graph(lrwb_prefix_subwords,
                     lrwb_stem_subwords,
                     lrwb_suffix_subwords,
                     lrwb_word_subwords);

    map<string, vector<string> > testWords;
    LRWBGraphTestUtils::word_subwords(lrwb_word_subwords, testWords);
    LRWBGraphTestUtils::pre_suf_words(lrwb_prefix_subwords,
                                      lrwb_suffix_subwords,
                                      testWords);
    cerr << "Number of words to verify: " << testWords.size() << endl;
    BOOST_CHECK( swg.assert_words(testWords) );
    BOOST_CHECK( swg.assert_word_pairs(testWords, true, false) ); //short sil, wb symbol
}


// All subword types
// No one phone subwords
BOOST_AUTO_TEST_CASE(LRWBSubwordGraphTest3)
{
    cerr << endl;
    LRWBSubwordGraph swg;
    LRWBGraphTestUtils::read_fixtures(swg, "data/lrwb_3.lex");

    swg.create_graph(lrwb_prefix_subwords,
                     lrwb_stem_subwords,
                     lrwb_suffix_subwords,
                     lrwb_word_subwords);

    map<string, vector<string> > testWords;
    LRWBGraphTestUtils::word_subwords(lrwb_word_subwords, testWords);
    LRWBGraphTestUtils::pre_suf_words(lrwb_prefix_subwords,
                                      lrwb_suffix_subwords,
                                      testWords);
    LRWBGraphTestUtils::pre_stem_suf_words(lrwb_prefix_subwords,
                                           lrwb_stem_subwords,
                                           lrwb_suffix_subwords,
                                           testWords);
    cerr << "Number of words to verify: " << testWords.size() << endl;
    BOOST_CHECK( swg.assert_words(testWords) );
    BOOST_CHECK( swg.assert_word_pairs(testWords, true, false) ); //short sil, wb symbol
}


// All subword types
// One phone stem subword
BOOST_AUTO_TEST_CASE(LRWBSubwordGraphTest4)
{
    cerr << endl;
    LRWBSubwordGraph swg;
    LRWBGraphTestUtils::read_fixtures(swg, "data/lrwb_4.lex");

    swg.create_graph(lrwb_prefix_subwords,
                     lrwb_stem_subwords,
                     lrwb_suffix_subwords,
                     lrwb_word_subwords);

    map<string, vector<string> > testWords;
    LRWBGraphTestUtils::word_subwords(lrwb_word_subwords, testWords);
    LRWBGraphTestUtils::pre_suf_words(lrwb_prefix_subwords,
                                      lrwb_suffix_subwords,
                                      testWords);
    LRWBGraphTestUtils::pre_stem_suf_words(lrwb_prefix_subwords,
                                           lrwb_stem_subwords,
                                           lrwb_suffix_subwords,
                                           testWords);
    cerr << "Number of words to verify: " << testWords.size() << endl;
    BOOST_CHECK( swg.assert_words(testWords) );
    BOOST_CHECK( swg.assert_word_pairs(testWords, true, false) ); //short sil, wb symbol
}


// All subword types
// One phone prefix, stem and suffix subwords
BOOST_AUTO_TEST_CASE(LRWBSubwordGraphTest5)
{
    cerr << endl;
    LRWBSubwordGraph swg;
    LRWBGraphTestUtils::read_fixtures(swg, "data/lrwb_5.lex");

    swg.create_graph(lrwb_prefix_subwords,
                     lrwb_stem_subwords,
                     lrwb_suffix_subwords,
                     lrwb_word_subwords);

    map<string, vector<string> > testWords;
    LRWBGraphTestUtils::word_subwords(lrwb_word_subwords, testWords);
    LRWBGraphTestUtils::pre_suf_words(lrwb_prefix_subwords,
                                      lrwb_suffix_subwords,
                                      testWords);
    LRWBGraphTestUtils::pre_stem_suf_words(lrwb_prefix_subwords,
                                           lrwb_stem_subwords,
                                           lrwb_suffix_subwords,
                                           testWords);
    cerr << "Number of words to verify: " << testWords.size() << endl;
    BOOST_CHECK( swg.assert_words(testWords) );
    BOOST_CHECK( swg.assert_word_pairs(testWords, true, false) ); //short sil, wb symbol
}


// Simplified problem case
/*
BOOST_AUTO_TEST_CASE(LRWBSubwordGraphTest6)
{
    cerr << endl;
    LRWBSubwordGraph swg;
    LRWBGraphTestUtils::read_fixtures(swg, "data/lrwb_6.lex");

    swg.create_graph(lrwb_prefix_subwords,
                     lrwb_stem_subwords,
                     lrwb_suffix_subwords,
                     lrwb_word_subwords);

    map<string, vector<string> > testWords;
    LRWBGraphTestUtils::word_subwords(lrwb_word_subwords, testWords);
    LRWBGraphTestUtils::pre_suf_words(lrwb_prefix_subwords,
                                      lrwb_suffix_subwords,
                                      testWords);
    LRWBGraphTestUtils::pre_stem_suf_words(lrwb_prefix_subwords,
                                           lrwb_stem_subwords,
                                           lrwb_suffix_subwords,
                                           testWords);
    cerr << "Number of words to verify: " << testWords.size() << endl;
    BOOST_CHECK( swg.assert_words(testWords) );
    BOOST_CHECK( swg.assert_word_pairs(testWords, true, false) ); //short sil, wb symbol
}
*/

//ofstream origoutf("acw.dot");
//swg.print_dot_digraph(swg.m_nodes, origoutf, true);
//origoutf.close();

