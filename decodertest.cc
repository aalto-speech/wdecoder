#include <string>
#include <iostream>
#include <fstream>

#include "decodertest.hh"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace std;


CPPUNIT_TEST_SUITE_REGISTRATION (decodertest);


void decodertest::setUp (void)
{
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_dgraph("data/swsw.test.graph");
}


void decodertest::tearDown (void)
{
}


void decodertest::DecoderTest1(void)
{
    d.set_la_state_indices_to_nodes();
    d.set_la_state_successor_lists();

    for (int i=0; i<d.m_nodes.size(); i++) {
        int la_state_idx = d.m_nodes[i].la_state_idx;
        set<int> &computed_successors = d.m_la_state_successor_words[la_state_idx];
        set<int> successors;
        d.find_successor_words(i, successors, true);
        CPPUNIT_ASSERT( computed_successors == successors );
    }
}
