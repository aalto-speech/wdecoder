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

    vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
    for (auto swit = dg.m_lexicon.begin(); swit != dg.m_lexicon.end(); ++swit) {
        if (swit->second.size() < 2) continue;
        vector<DecoderGraph::TriphoneNode> sw_triphones;
        triphonize_subword(dg, swit->first, sw_triphones);
        dg.add_triphones(triphone_nodes, sw_triphones);
    }

    vector<DecoderGraph::Node> nodes(2);
    dg.triphones_to_states(triphone_nodes, nodes);
    triphone_nodes.clear();
    dg.prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    dg.create_crossword_network_for_subwords(cw_nodes, fanout, fanin);
    dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
    dg.connect_end_to_start_node(nodes);

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
