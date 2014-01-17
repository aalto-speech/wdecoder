#include <string>
#include <iostream>

#include "graphtest.hh"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace std;


CPPUNIT_TEST_SUITE_REGISTRATION (graphtest);

void graphtest :: setUp (void)
{
}

void graphtest :: tearDown (void)
{
}


void graphtest :: triphonize(string word,
                             vector<string> &triphones) {
    string tword = "_" + word + "_";
    triphones.clear();
    for (int i=1; i<tword.length()-1; i++) {
        stringstream tstring;
        tstring << tword[i-1] << "-" << tword[i] << "+" << tword[i+1];
        triphones.push_back(tstring.str());
    }
}


void graphtest :: GraphTest1(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    CPPUNIT_ASSERT_EQUAL( 35003, (int)dg.m_units.size() );
    CPPUNIT_ASSERT_EQUAL( 35003, (int)dg.m_unit_map.size() ); // ?
    CPPUNIT_ASSERT_EQUAL( 13252, (int)dg.m_hmms.size() );
    CPPUNIT_ASSERT_EQUAL( 13252, (int)dg.m_hmm_map.size() );
    CPPUNIT_ASSERT_EQUAL( 1170, (int)dg.m_hmm_states.size() );
    CPPUNIT_ASSERT_EQUAL( 9, (int)dg.m_word_segs.size() );

    vector<DecoderGraph::SubwordNode> nodes;
    dg.create_word_graph(nodes);
    CPPUNIT_ASSERT_EQUAL( 12, dg.reachable_word_graph_nodes(nodes) );
}


void graphtest :: GraphTest2(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    vector<DecoderGraph::SubwordNode> nodes;
    dg.create_word_graph(nodes);
    CPPUNIT_ASSERT_EQUAL( 12, dg.reachable_word_graph_nodes(nodes) );
    dg.tie_word_graph_suffixes(nodes);
    CPPUNIT_ASSERT_EQUAL( 11, dg.reachable_word_graph_nodes(nodes) );
}


void graphtest :: GraphTest3(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_word_graph_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes(2);
    dg.expand_subword_nodes(swnodes, nodes);
    CPPUNIT_ASSERT_EQUAL( 147, (int)nodes.size() );


}
