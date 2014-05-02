#include <string>
#include <iostream>
#include <fstream>

#include "swgraphtest.hh"
#include "gutils.hh"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

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


void swgraphtest::SubwordGraphTest1(void)
{
    DecoderGraph dg;
    read_fixtures(dg);
}


void swgraphtest::SubwordGraphTest2(void)
{
    DecoderGraph dg;
    read_fixtures(dg);
}


void swgraphtest::SubwordGraphTest3(void)
{
    DecoderGraph dg;
    read_fixtures(dg);
}


void swgraphtest::SubwordGraphTest4(void)
{
    DecoderGraph dg;
    read_fixtures(dg);
}


void swgraphtest::SubwordGraphTest5(void)
{
    DecoderGraph dg;
    read_fixtures(dg);
}
