
#include "wgraphtest.hh"
#include "gutils.hh"
#include "WordGraphBuilder.hh"


using namespace std;
using namespace gutils;


CPPUNIT_TEST_SUITE_REGISTRATION (wgraphtest);


void wgraphtest::setUp (void)
{
    amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
    lexname = string("data/lex");
}


void wgraphtest::tearDown (void)
{
}


void wgraphtest::read_fixtures(DecoderGraph &dg)
{
    dg.read_phone_model(amname + ".ph");
    dg.read_noway_lexicon(lexname);
}



void wgraphtest::make_graph(DecoderGraph &dg,
                           vector<DecoderGraph::Node> &nodes)
{

}


// Test tying state chain prefixes
void wgraphtest::WordGraphTest1(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    vector<DecoderGraph::Node> nodes(2);
    make_graph(dg, nodes);
}

