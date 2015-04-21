
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
    prefix_subwords.clear();
    subwords.clear();

    for (auto swit = swg.m_lexicon.begin(); swit != swg.m_lexicon.end(); ++swit) {
        if (swit->first.find("<") != string::npos) continue;
        if (swit->first.length() == 0) continue;
        if ((swit->first)[0] == '_')
            prefix_subwords.insert(swit->first);
        else subwords.insert(swit->first);
    }
}


void nowbswgraphtest::construct_words(const set<string> &prefix_subwords,
                                      const set<string> &subwords,
                                      map<string, vector<string> > &word_segs)
{
    word_segs.clear();
    for (auto wssit = prefix_subwords.begin(); wssit != prefix_subwords.end(); ++wssit) {
        for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
            vector<string> word_seg;
            word_seg.push_back(*wssit);
            word_seg.push_back(*swit);
            string wrd = *wssit + *swit;
            word_segs[wrd] = word_seg;
        }
    }
}


void nowbswgraphtest::construct_complex_words(const set<string> &prefix_subwords,
                                              const set<string> &subwords,
                                              map<string, vector<string> > &word_segs)
{
    word_segs.clear();
    for (auto wssit = prefix_subwords.begin(); wssit != prefix_subwords.end(); ++wssit) {
        for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
            for (auto eswit = subwords.begin(); eswit != subwords.end(); ++eswit) {
                vector<string> word_seg;
                word_seg.push_back(*wssit);
                word_seg.push_back(*swit);
                word_seg.push_back(*eswit);
                string wrd = *wssit + *swit + *eswit;
                word_segs[wrd] = word_seg;
            }
        }
    }
}


void nowbswgraphtest::NoWBSubwordGraphTest1(void)
{
    NoWBSubwordGraph swg;
    lexname = "data/nowb_1.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, subwords);

    CPPUNIT_ASSERT( swg.assert_words(prefix_subwords) );
    CPPUNIT_ASSERT( swg.assert_only_words(prefix_subwords) );
}


void nowbswgraphtest::NoWBSubwordGraphTest2(void)
{
    NoWBSubwordGraph swg;
    lexname = "data/nowb_1.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, subwords);

    CPPUNIT_ASSERT( swg.assert_words(prefix_subwords) );
    CPPUNIT_ASSERT( swg.assert_only_words(prefix_subwords) );

    map<string, vector<string> > word_segs;
    construct_words(prefix_subwords, subwords, word_segs);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
}


void nowbswgraphtest::NoWBSubwordGraphTest3(void)
{
    NoWBSubwordGraph swg;
    lexname = "data/nowb_1.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, subwords);

    CPPUNIT_ASSERT( swg.assert_words(prefix_subwords) );
    CPPUNIT_ASSERT( swg.assert_only_words(prefix_subwords) );

    map<string, vector<string> > word_segs;
    construct_complex_words(prefix_subwords, subwords, word_segs);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
}


void nowbswgraphtest::NoWBSubwordGraphTest4(void)
{
    NoWBSubwordGraph swg;
    lexname = "data/nowb_1.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, subwords);

    CPPUNIT_ASSERT( swg.assert_words(prefix_subwords) );
    CPPUNIT_ASSERT( swg.assert_only_words(prefix_subwords) );

    map<string, vector<string> > word_segs;
    construct_complex_words(prefix_subwords, subwords, word_segs);

    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, false) ); //short sil, wb symbol
}


void nowbswgraphtest::NoWBSubwordGraphTest5(void)
{
    NoWBSubwordGraph swg;
    lexname = "data/nowb_2.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, subwords);

    map<string, vector<string> > word_segs;
    construct_words(prefix_subwords, subwords, word_segs);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
}


void nowbswgraphtest::NoWBSubwordGraphTest6(void)
{
    NoWBSubwordGraph swg;
    lexname = "data/nowb_3.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, subwords);

    map<string, vector<string> > word_segs;
    construct_words(prefix_subwords, subwords, word_segs);

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
}


void nowbswgraphtest::NoWBSubwordGraphTest7(void)
{
    NoWBSubwordGraph swg;
    lexname = "data/nowb_4.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, subwords);

    map<string, vector<string> > word_segs;
    word_segs["_atooppinen"] = {"_a", "tooppinen"};
    word_segs["_atopia"] = {"_a", "topi", "a"};
    word_segs["_aie"] = {"_a", "i", "e"};
    word_segs["_aito"] = {"_a", "i", "to"};
    word_segs["_ai"] = {"_a", "i"};
    word_segs["_atopiaa"] = {"_a", "topi", "a", "a"};

    CPPUNIT_ASSERT( swg.assert_words(word_segs) );
}


void nowbswgraphtest::NoWBSubwordGraphTest8(void)
{
    NoWBSubwordGraph swg;
    lexname = "data/nowb_4.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, subwords);

    map<string, vector<string> > word_segs;
    word_segs["_atooppinen"] = {"_a", "tooppinen"};
    word_segs["_atopia"] = {"_a", "topi", "a"};
    word_segs["_aie"] = {"_a", "i", "e"};
    word_segs["_aito"] = {"_a", "i", "to"};
    word_segs["_ai"] = {"_a", "i"};
    word_segs["_atopiaa"] = {"_a", "topi", "a", "a"};

    CPPUNIT_ASSERT( swg.assert_word_pairs(word_segs, true, false) ); //short sil, wb symbol
}


//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
