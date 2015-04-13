
#include "swgraphtest.hh"
#include "gutils.hh"
#include "SubwordGraphBuilder.hh"


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


void swgraphtest::read_fixtures(DecoderGraph &dg,
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
    DecoderGraph dg;
    read_fixtures(dg, "data/segs.txt");

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, subwords);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest2(void)
{
    DecoderGraph dg;
    read_fixtures(dg, "data/segs2.txt");

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, subwords);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest3(void)
{
    DecoderGraph dg;
    read_fixtures(dg, "data/segs3.txt");

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, subwords);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest4(void)
{
    DecoderGraph dg;
    read_fixtures(dg, "data/segs4.txt");

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, subwords);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest5(void)
{
    DecoderGraph dg;
    read_fixtures(dg, "data/segs5.txt");

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, subwords);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest6(void)
{
    DecoderGraph dg;
    read_fixtures(dg, "data/segs6.txt");

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, subwords);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest7(void)
{
    DecoderGraph dg;
    read_fixtures(dg, "data/subword_tie_expand_problem.segs");

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, subwords);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest8(void)
{
    DecoderGraph dg;
    read_fixtures(dg, "data/500.segs");

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, subwords);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
