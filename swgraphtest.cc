
#include "swgraphtest.hh"
#include "gutils.hh"
#include "SubwordGraph.hh"


using namespace std;
using namespace gutils;


CPPUNIT_TEST_SUITE_REGISTRATION (swgraphtest);


void swgraphtest::setUp (void)
{
    amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
    lexname = string("data/lex");
}


void swgraphtest::tearDown (void)
{
}


void swgraphtest::read_fixtures(SubwordGraph &dg,
                                string segfname)
{
    dg.read_phone_model(amname + ".ph");
    dg.read_noway_lexicon(lexname);
    word_segs.clear();
    subwords.clear();
    read_word_segmentations(dg, segfname, word_segs);
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit)
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit)
            subwords.insert(*swit);
}


// Normal case
void swgraphtest::SubwordGraphTest1(void)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs.txt");

    swg.create_graph(subwords);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, true) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest2(void)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs2.txt");

    swg.create_graph(subwords);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, true) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest3(void)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs3.txt");

    swg.create_graph(subwords);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, true) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest4(void)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs4.txt");

    swg.create_graph(subwords);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, true) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest5(void)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs5.txt");

    swg.create_graph(subwords);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, true) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest6(void)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs6.txt");

    swg.create_graph(subwords);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, true) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest7(void)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/subword_tie_expand_problem.segs");

    swg.create_graph(subwords);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, true) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest8(void)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/500.segs");

    swg.create_graph(subwords);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, true) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, false, false) );
}


//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
