
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


void swgraphtest::read_fixtures(DecoderGraph &dg)
{
    dg.read_phone_model(amname + ".ph");
    dg.read_noway_lexicon(lexname);
}


// Normal case
void swgraphtest::SubwordGraphTest1(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest2(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs2.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest3(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs3.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest4(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs4.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest5(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs5.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest6(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs6.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest7(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/subword_tie_expand_problem.segs";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest8(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/500.segs";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    subwordgraphbuilder::create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
