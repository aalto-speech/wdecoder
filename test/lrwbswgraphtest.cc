#include <boost/test/unit_test.hpp>

#include "LRWBSubwordGraph.hh"

using namespace std;

string lrwb_amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
set<string> lrwb_prefix_subwords;
set<string> lrwb_stem_subwords;
set<string> lrwb_suffix_subwords;
set<string> lrwb_word_subwords;

namespace LRWBGraphTestUtils {
    void read_fixtures(LRWBSubwordGraph &swg, string lexfname) {
        swg.read_phone_model(lrwb_amname + ".ph");
        swg.read_lexicon(lexfname,
                         lrwb_prefix_subwords,
                         lrwb_stem_subwords,
                         lrwb_suffix_subwords,
                         lrwb_word_subwords);
    }
};

// Only complete word subwords
BOOST_AUTO_TEST_CASE(LRWBSubwordGraphTest1)
{
    LRWBSubwordGraph swg;
    LRWBGraphTestUtils::read_fixtures(swg, "data/lrwb_1.lex");

    swg.create_graph(lrwb_prefix_subwords,
                     lrwb_stem_subwords,
                     lrwb_suffix_subwords,
                     lrwb_word_subwords);

    BOOST_CHECK( swg.assert_words(lrwb_word_subwords) );
    BOOST_CHECK( swg.assert_only_words(lrwb_word_subwords) );
    BOOST_CHECK( swg.assert_word_pairs(lrwb_word_subwords, true, false) ); //short sil, wb symbol
}


//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
