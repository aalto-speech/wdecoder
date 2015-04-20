
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


void nowbswgraphtest::read_fixtures(NoWBSubwordGraph &swg)
{
    swg.read_phone_model(amname + ".ph");
    swg.read_noway_lexicon(lexname);
    word_start_subwords.clear();
    subwords.clear();

    for (auto swit = swg.m_lexicon.begin(); swit != swg.m_lexicon.end(); ++swit) {
        if (swit->first.find("<") != string::npos) continue;
        if (swit->first.length() == 0) continue;
        if ((swit->first)[0] == '_')
            word_start_subwords.insert(swit->first);
        else subwords.insert(swit->first);
    }
}


// Normal case
void nowbswgraphtest::NoWBSubwordGraphTest1(void)
{
    NoWBSubwordGraph swg;
    lexname = "data/nowb_1.lex";
    read_fixtures(swg);

    swg.create_graph(subwords);

    CPPUNIT_ASSERT( swg.assert_words(subwords) );
}

//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
