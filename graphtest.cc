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

    CPPUNIT_ASSERT_EQUAL( 35004, (int)dg.m_units.size() );
    CPPUNIT_ASSERT_EQUAL( 35003, (int)dg.m_unit_map.size() ); // ?
    CPPUNIT_ASSERT_EQUAL( 13252, (int)dg.m_hmms.size() );
    CPPUNIT_ASSERT_EQUAL( 13252, (int)dg.m_hmm_map.size() );
    CPPUNIT_ASSERT_EQUAL( 1170, dg.m_num_models );
    CPPUNIT_ASSERT_EQUAL( 7, (int)dg.m_word_segs.size() );

    vector<DecoderGraph::SubwordNode> nodes;
    dg.create_word_graph(nodes);
    CPPUNIT_ASSERT_EQUAL( 10, (int)nodes.size() );
}
