#include <boost/test/unit_test.hpp>

#include "LWBSubwordGraph.hh"

using namespace std;


string amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
string lexname = string("data/lex");
set<string> prefix_subwords;
set<string> stem_subwords;

void read_fixtures(LWBSubwordGraph &swg)
{
    swg.read_phone_model(amname + ".ph");
    swg.read_noway_lexicon(lexname);
    prefix_subwords.clear();
    stem_subwords.clear();

    for (auto swit = swg.m_lexicon.begin(); swit != swg.m_lexicon.end(); ++swit) {
        if (swit->first.find("<") != string::npos) continue;
        if (swit->first.length() == 0) continue;
        if ((swit->first)[0] == '_')
            prefix_subwords.insert(swit->first);
        else stem_subwords.insert(swit->first);
    }
}


void construct_words(const set<string> &prefix_subwords,
                     const set<string> &subwords,
                     map<string, vector<string> > &word_segs)
{
    word_segs.clear();
    for (auto wssit = prefix_subwords.begin(); wssit != prefix_subwords.end(); ++wssit) {
        for (auto swit = stem_subwords.begin(); swit != stem_subwords.end(); ++swit) {
            vector<string> word_seg;
            word_seg.push_back(*wssit);
            word_seg.push_back(*swit);
            string wrd = *wssit + *swit;
            word_segs[wrd] = word_seg;
        }
    }
}


void construct_complex_words(const set<string> &prefix_subwords,
                             const set<string> &subwords,
                             map<string, vector<string> > &word_segs)
{
    word_segs.clear();
    for (auto wssit = prefix_subwords.begin(); wssit != prefix_subwords.end(); ++wssit) {
        for (auto swit = stem_subwords.begin(); swit != stem_subwords.end(); ++swit) {
            for (auto eswit = stem_subwords.begin(); eswit != stem_subwords.end(); ++eswit) {
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


BOOST_AUTO_TEST_CASE(NoWBSubwordGraphTest1)
{
    LWBSubwordGraph swg;
    lexname = "data/lwb_1.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, stem_subwords);

    BOOST_CHECK( swg.assert_words(prefix_subwords) );
    BOOST_CHECK( swg.assert_only_words(prefix_subwords) );
    BOOST_CHECK( swg.assert_word_pairs(prefix_subwords, true, false) ); //short sil, wb symbol
}


BOOST_AUTO_TEST_CASE(NoWBSubwordGraphTest2)
{
    LWBSubwordGraph swg;
    lexname = "data/lwb_1.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, stem_subwords);

    BOOST_CHECK( swg.assert_words(prefix_subwords) );
    BOOST_CHECK( swg.assert_only_words(prefix_subwords) );

    map<string, vector<string> > word_segs;
    construct_words(prefix_subwords, stem_subwords, word_segs);

    BOOST_CHECK( swg.assert_words(word_segs) );
}


BOOST_AUTO_TEST_CASE(NoWBSubwordGraphTest3)
{
    LWBSubwordGraph swg;
    lexname = "data/lwb_1.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, stem_subwords);

    BOOST_CHECK( swg.assert_words(prefix_subwords) );
    BOOST_CHECK( swg.assert_only_words(prefix_subwords) );

    map<string, vector<string> > word_segs;
    construct_complex_words(prefix_subwords, stem_subwords, word_segs);

    BOOST_CHECK( swg.assert_words(word_segs) );
}


BOOST_AUTO_TEST_CASE(NoWBSubwordGraphTest4)
{
    LWBSubwordGraph swg;
    lexname = "data/lwb_1.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, stem_subwords);

    BOOST_CHECK( swg.assert_words(prefix_subwords) );
    BOOST_CHECK( swg.assert_only_words(prefix_subwords) );

    map<string, vector<string> > word_segs;
    construct_complex_words(prefix_subwords, stem_subwords, word_segs);

    BOOST_CHECK( swg.assert_word_pairs(word_segs, true, false) ); //short sil, wb symbol
}


BOOST_AUTO_TEST_CASE(NoWBSubwordGraphTest5)
{
    LWBSubwordGraph swg;
    lexname = "data/lwb_2.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, stem_subwords);

    map<string, vector<string> > word_segs;
    construct_words(prefix_subwords, stem_subwords, word_segs);

    BOOST_CHECK( swg.assert_words(word_segs) );
}


BOOST_AUTO_TEST_CASE(NoWBSubwordGraphTest6)
{
    LWBSubwordGraph swg;
    lexname = "data/lwb_3.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, stem_subwords);

    map<string, vector<string> > word_segs;
    construct_words(prefix_subwords, stem_subwords, word_segs);

    BOOST_CHECK( swg.assert_words(word_segs) );
}


BOOST_AUTO_TEST_CASE(NoWBSubwordGraphTest7)
{
    LWBSubwordGraph swg;
    lexname = "data/lwb_4.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, stem_subwords);

    map<string, vector<string> > word_segs;
    word_segs["_atooppinen"] = {"_a", "tooppinen"};
    word_segs["_atopia"] = {"_a", "topi", "a"};
    word_segs["_aie"] = {"_a", "i", "e"};
    word_segs["_aito"] = {"_a", "i", "to"};
    word_segs["_ai"] = {"_a", "i"};
    word_segs["_atopiaa"] = {"_a", "topi", "a", "a"};

    BOOST_CHECK( swg.assert_words(word_segs) );
}


BOOST_AUTO_TEST_CASE(NoWBSubwordGraphTest8)
{
    LWBSubwordGraph swg;
    lexname = "data/lwb_4.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, stem_subwords);

    map<string, vector<string> > word_segs;
    word_segs["_atooppinen"] = {"_a", "tooppinen"};
    word_segs["_atopia"] = {"_a", "topi", "a"};
    word_segs["_aie"] = {"_a", "i", "e"};
    word_segs["_aito"] = {"_a", "i", "to"};
    word_segs["_ai"] = {"_a", "i"};
    word_segs["_atopiaa"] = {"_a", "topi", "a", "a"};

    BOOST_CHECK( swg.assert_word_pairs(word_segs, true, false) ); //short sil, wb symbol
}


BOOST_AUTO_TEST_CASE(NoWBSubwordGraphTest9)
{
    LWBSubwordGraph swg;
    lexname = "data/lwb_5.lex";
    read_fixtures(swg);

    swg.create_graph(prefix_subwords, stem_subwords);

    map<string, vector<string> > word_segs;
    word_segs["_iho"] = {"_iho"};
    word_segs["_atooppinen"] = {"_a", "tooppinen"};
    word_segs["_atopia"] = {"_a", "topi", "a"};
    word_segs["_aie"] = {"_a", "i", "e"};
    word_segs["_aito"] = {"_a", "i", "to"};
    word_segs["_ai"] = {"_a", "i"};
    word_segs["_atopiaa"] = {"_a", "topi", "a", "a"};

    BOOST_CHECK( swg.assert_words(word_segs) );
    BOOST_CHECK( swg.assert_word_pairs(word_segs, true, false) ); //short sil, wb symbol
}


BOOST_AUTO_TEST_CASE(NoWBSubwordGraphTest10)
{
    LWBSubwordGraph swg;
    lexname = "data/lwb_20k.lex";
    read_fixtures(swg);

    cerr << endl;
    swg.create_graph(prefix_subwords, stem_subwords, true);
    cerr << "number of arcs: " << DecoderGraph::num_arcs(swg.m_nodes) << endl;

    map<string, vector<string> > word_segs;
    swg.read_word_segmentations("data/lwb_20k.wsegs", word_segs);

    cerr << "Asserting words.." << endl;
    BOOST_CHECK( swg.assert_words(word_segs) );
    cerr << "Asserting word pairs.." << endl;
    BOOST_CHECK( swg.assert_word_pairs(word_segs, 50000, true, false) ); //short sil, wb symbol
}


//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
