
#include "nowbswgraphtest.hh"
#include "NoWBSubwordGraph.hh"

using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION (nowbswgraphtest);


void nowbswgraphtest::setUp (void)
{
    amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
    lexname = string("data/lex");
}


void nowbswgraphtest::tearDown (void)
{
}


void nowbswgraphtest::read_fixtures(NoWBSubwordGraph &swg,
                                    string segfname)
{
    swg.read_phone_model(amname + ".ph");
    swg.read_noway_lexicon(lexname);
    word_segs.clear();
    subwords.clear();
    swg.read_word_segmentations(segfname, word_segs);
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit)
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit)
            subwords.insert(*swit);
}


// Normal case
void nowbswgraphtest::NoWBSubwordGraphTest1(void)
{
    NoWBSubwordGraph swg;
    read_fixtures(swg, "data/segs.txt");

    swg.create_graph(subwords);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, true) );
    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, false, false) );
}

//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
