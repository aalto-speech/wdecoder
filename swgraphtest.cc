
#include "swgraphtest.hh"
#include "gutils.hh"
#include "SubwordGraphBuilder.hh"


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


void swgraphtest::create_graph(DecoderGraph &dg,
                               vector<DecoderGraph::Node> &nodes,
                               map<string, vector<string> > &word_segs)
{
    set<string> subwords;
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit)
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit)
            subwords.insert(*swit);

    nodes.clear(); nodes.resize(2);
    for (auto swit = subwords.begin(); swit != subwords.end(); ++swit) {
        // FIXME: should check number of phones
        if (swit->length() < 2) continue;
        vector<TriphoneNode> subword_triphones;
        triphonize_subword(dg, *swit, subword_triphones);
        vector<DecoderGraph::Node> subword_nodes;
        triphones_to_state_chain(dg, subword_triphones, subword_nodes);
        subword_nodes[3].from_fanin.insert(dg.m_lexicon[*swit][0]);
        subword_nodes[subword_nodes.size()-4].to_fanout.insert(dg.m_lexicon[*swit].back());
        add_nodes_to_tree(dg, nodes, subword_nodes);
    }

    lookahead_to_arcs(nodes);

    prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    subwordgraphbuilder::create_crossword_network(dg, subwords, cw_nodes, fanout, fanin);
    subwordgraphbuilder::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin, false);
    connect_end_to_start_node(nodes);
    subwordgraphbuilder::connect_one_phone_subwords_from_start_to_cw(dg, subwords, nodes, fanout);
    subwordgraphbuilder::connect_one_phone_subwords_from_cw_to_end(dg, subwords, nodes, fanin);
    prune_unreachable_nodes(nodes);

    push_word_ids_right(nodes);
    tie_state_prefixes(nodes);
    push_word_ids_left(nodes);
    tie_state_suffixes(nodes);
}


// Normal case
void swgraphtest::SubwordGraphTest1(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest2(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs2.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest3(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs3.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest4(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs4.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest5(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs5.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest6(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs6.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest7(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/subword_tie_expand_problem.segs";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


void swgraphtest::SubwordGraphTest8(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/500.segs";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);
    create_graph(dg, nodes, word_segs);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false) );
}


//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
