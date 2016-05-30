#include <boost/test/unit_test.hpp>

#include "SubwordGraph.hh"

using namespace std;


map<string, vector<string> > _word_segs;
set<string> subwords;


void read_fixtures(SubwordGraph &swg,
                   string segfname)
{
    swg.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    swg.read_noway_lexicon("data/lex");
    _word_segs.clear();
    subwords.clear();
    swg.read_word_segmentations(segfname, _word_segs);
    for (auto wit = _word_segs.begin(); wit != _word_segs.end(); ++wit)
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit)
            subwords.insert(*swit);
}


// Normal case
BOOST_AUTO_TEST_CASE(SubwordGraphTest1)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs.txt");

    swg.create_graph(subwords);

    BOOST_CHECK( swg.assert_words(_word_segs) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, true, true) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, false, false) );
}


BOOST_AUTO_TEST_CASE(SubwordGraphTest2)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs2.txt");

    swg.create_graph(subwords);

    BOOST_CHECK( swg.assert_words(_word_segs) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, true, true) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, false, false) );
}


BOOST_AUTO_TEST_CASE(SubwordGraphTest3)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs3.txt");

    swg.create_graph(subwords);

    BOOST_CHECK( swg.assert_words(_word_segs) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, true, true) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, false, false) );
}


BOOST_AUTO_TEST_CASE(SubwordGraphTest4)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs4.txt");

    swg.create_graph(subwords);

    BOOST_CHECK( swg.assert_words(_word_segs) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, true, true) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, false, false) );
}


BOOST_AUTO_TEST_CASE(SubwordGraphTest5)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs5.txt");

    swg.create_graph(subwords);

    BOOST_CHECK( swg.assert_words(_word_segs) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, true, true) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, false, false) );
}


BOOST_AUTO_TEST_CASE(SubwordGraphTest6)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/segs6.txt");

    swg.create_graph(subwords);

    BOOST_CHECK( swg.assert_words(_word_segs) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, true, true) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, false, false) );
}


BOOST_AUTO_TEST_CASE(SubwordGraphTest7)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/subword_tie_expand_problem.segs");

    swg.create_graph(subwords);

    BOOST_CHECK( swg.assert_words(_word_segs) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, true, true) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, false, false) );
}


BOOST_AUTO_TEST_CASE(SubwordGraphTest8)
{
    SubwordGraph swg;
    read_fixtures(swg, "data/500.segs");

    swg.create_graph(subwords);

    BOOST_CHECK( swg.assert_words(_word_segs) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, true, true) );
    BOOST_CHECK( swg.assert_word_pairs(_word_segs, false, false) );
}


//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
