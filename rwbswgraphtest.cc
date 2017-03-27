#include <boost/test/unit_test.hpp>

#include "RWBSubwordGraph.hh"

using namespace std;


string _amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
string _lexname = string("data/lex");
set<string> _stem_subwords;
set<string> _suffix_subwords;

namespace RWBGraphTestUtils {

    void read_fixtures(RWBSubwordGraph &swg) {
        swg.read_phone_model(_amname + ".ph");
        swg.read_noway_lexicon(_lexname);
        _suffix_subwords.clear();
        _stem_subwords.clear();

        for (auto swit = swg.m_lexicon.begin(); swit != swg.m_lexicon.end(); ++swit) {
            if (swit->first.find("<") != string::npos) continue;
            if (swit->first.length() == 0) continue;
            if (swit->first.back() == '_')
                _suffix_subwords.insert(swit->first);
            else _stem_subwords.insert(swit->first);
        }
    }


    void construct_words(const set<string> &prefix_subwords,
                         const set<string> &subwords,
                         map<string, vector<string> > &word_segs) {
        word_segs.clear();
        for (auto stemit = _stem_subwords.begin(); stemit != _stem_subwords.end(); ++stemit) {
            for (auto suffit = _suffix_subwords.begin(); suffit != _suffix_subwords.end(); ++suffit) {
                vector<string> word_seg;
                word_seg.push_back(*stemit);
                word_seg.push_back(*suffit);
                string wrd = *stemit + *suffit;
                word_segs[wrd] = word_seg;
            }
        }
    }


    void construct_complex_words(const set<string> &prefix_subwords,
                                 const set<string> &subwords,
                                 map<string, vector<string> > &word_segs) {
        word_segs.clear();
        for (auto stemit = _stem_subwords.begin(); stemit != _stem_subwords.end(); ++stemit) {
            for (auto stemit2 = _stem_subwords.begin(); stemit2 != _stem_subwords.end(); ++stemit2) {
                for (auto suffit = _suffix_subwords.begin(); suffit != _suffix_subwords.end(); ++suffit) {
                    vector<string> word_seg;
                    word_seg.push_back(*stemit);
                    word_seg.push_back(*stemit2);
                    word_seg.push_back(*suffit);
                    string wrd = *stemit + *stemit2 + *suffit;
                    word_segs[wrd] = word_seg;
                }
            }
        }
    }
};


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest1)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_1.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    swg.create_graph(_stem_subwords, _suffix_subwords);

    BOOST_CHECK( swg.assert_words(_suffix_subwords) );
    BOOST_CHECK( swg.assert_only_words(_suffix_subwords) );
    BOOST_CHECK( swg.assert_word_pairs(_suffix_subwords, true, false) ); //short sil, wb symbol
}


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest2)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_1.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    swg.create_graph(_stem_subwords, _suffix_subwords);

    BOOST_CHECK( swg.assert_words(_suffix_subwords) );
    BOOST_CHECK( swg.assert_only_words(_suffix_subwords) );

    map<string, vector<string> > word_segs;
    RWBGraphTestUtils::construct_words(_stem_subwords, _suffix_subwords, word_segs);

    BOOST_CHECK( swg.assert_words(word_segs) );
}


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest3)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_1.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    swg.create_graph(_stem_subwords, _suffix_subwords);

    BOOST_CHECK( swg.assert_words(_suffix_subwords) );
    BOOST_CHECK( swg.assert_only_words(_suffix_subwords) );

    map<string, vector<string> > word_segs;
    RWBGraphTestUtils::construct_complex_words(_stem_subwords, _suffix_subwords, word_segs);

    BOOST_CHECK( swg.assert_words(word_segs) );
}


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest4)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_1.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    swg.create_graph(_stem_subwords, _suffix_subwords);

    BOOST_CHECK( swg.assert_words(_suffix_subwords) );
    BOOST_CHECK( swg.assert_only_words(_suffix_subwords) );

    map<string, vector<string> > word_segs;
    RWBGraphTestUtils::construct_complex_words(_stem_subwords, _suffix_subwords, word_segs);

    BOOST_CHECK( swg.assert_word_pairs(word_segs, true, false) ); //short sil, wb symbol
}


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest5)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_2.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    swg.create_graph(_stem_subwords, _suffix_subwords);

    map<string, vector<string> > word_segs;
    RWBGraphTestUtils::construct_words(_stem_subwords, _suffix_subwords, word_segs);

    BOOST_CHECK( swg.assert_words(word_segs) );
}


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest6)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_3.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    swg.create_graph(_stem_subwords, _suffix_subwords);

    map<string, vector<string> > word_segs;
    RWBGraphTestUtils::construct_words(_stem_subwords, _suffix_subwords, word_segs);

    BOOST_CHECK( swg.assert_words(word_segs) );
}


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest7)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_4.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    swg.create_graph(_stem_subwords, _suffix_subwords);

    map<string, vector<string> > word_segs;
    word_segs["atooppinen_"] = {"a", "tooppinen_"};
    word_segs["atopia_"] = {"a", "topi", "a_"};
    word_segs["aie_"] = {"a", "i", "e_"};
    word_segs["aito_"] = {"a", "i", "to_"};
    word_segs["ai_"] = {"a", "i_"};
    word_segs["atopiaa_"] = {"a", "topi", "a", "a_"};

    BOOST_CHECK( swg.assert_words(word_segs) );
}


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest8)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_4.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    swg.create_graph(_stem_subwords, _suffix_subwords);

    map<string, vector<string> > word_segs;
    word_segs["atooppinen_"] = {"a", "tooppinen_"};
    word_segs["atopia_"] = {"a", "topi", "a_"};
    word_segs["aie_"] = {"a", "i", "e_"};
    word_segs["aito_"] = {"a", "i", "to_"};
    word_segs["ai_"] = {"a", "i_"};
    word_segs["atopiaa_"] = {"a", "topi", "a", "a_"};

    BOOST_CHECK( swg.assert_word_pairs(word_segs, true, false) ); //short sil, wb symbol
}


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest9)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_5.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    swg.create_graph(_stem_subwords, _suffix_subwords);
    BOOST_CHECK_EQUAL( 0, DecoderGraph::num_nodes_with_no_arcs(swg.m_nodes) );

    map<string, vector<string> > word_segs;
    word_segs["iho_"] = {"iho_"};
    word_segs["atooppinen_"] = {"a", "tooppinen_"};
    word_segs["atopia_"] = {"a", "topi", "a_"};
    word_segs["aie_"] = {"a", "i", "e_"};
    word_segs["aito_"] = {"a", "i", "to_"};
    word_segs["ai_"] = {"a", "i_"};
    word_segs["atopiaa_"] = {"a", "topi", "a", "a_"};

    BOOST_CHECK( swg.assert_words(word_segs) );
    BOOST_CHECK( swg.assert_word_pairs(word_segs, true, false) ); //short sil, wb symbol
}


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest10)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_5.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    swg.create_graph(_stem_subwords, _suffix_subwords);
    BOOST_CHECK_EQUAL( 0, DecoderGraph::num_nodes_with_no_arcs(swg.m_nodes) );

    map<string, vector<string> > word_segs;
    word_segs["topi"] = {"topi"};
    word_segs["a"] = {"a"};
    word_segs["ai"] = {"a", "i"};
    word_segs["ia"] = {"i", "a"};
    word_segs["atopi"] = {"a", "topi"};
    word_segs["topia"] = {"topi", "a"};

    BOOST_CHECK( swg.assert_words_not_in_graph(word_segs) );
}


BOOST_AUTO_TEST_CASE(RWBSubwordGraphTest11)
{
    RWBSubwordGraph swg;
    _lexname = "data/rwb_20k.lex";
    RWBGraphTestUtils::read_fixtures(swg);

    cerr << endl;
    swg.create_graph(_stem_subwords, _suffix_subwords, true);
    cerr << "number of arcs: " << DecoderGraph::num_arcs(swg.m_nodes) << endl;
    BOOST_CHECK_EQUAL( 0, DecoderGraph::num_nodes_with_no_arcs(swg.m_nodes) );

    map<string, vector<string> > word_segs;
    swg.read_word_segmentations("data/rwb_20k.wsegs", word_segs);

    cerr << "Asserting words.." << endl;
    BOOST_CHECK( swg.assert_words(word_segs) );
    cerr << "Asserting word pairs.." << endl;
    BOOST_CHECK( swg.assert_word_pairs(word_segs, 50000, true, false) ); //short sil, wb symbol
}


//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
