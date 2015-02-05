
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
    vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
    set<string> subwords;
    nodes.clear();
    nodes.resize(2);
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit)
    {
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit)
        {
            subwords.insert(*swit);
            if (swit->length() < 2) continue;
            vector<DecoderGraph::TriphoneNode> word_triphones;
            triphonize_subword(dg, *swit, word_triphones);
            vector<DecoderGraph::Node> word_nodes;
            triphones_to_state_chain(dg, word_triphones, word_nodes);
            add_nodes_to_tree(dg, nodes, word_nodes);
        }
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

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false, false) );
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

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false, false) );
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

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false, false) );
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

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false, false) );
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

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false, false) );
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

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false, false) );
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

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false, false) );
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

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false, false) );
}


void swgraphtest::SubwordGraphTest9(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/500.segs";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);

    vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
    set<string> subwords;
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit)
    {
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit)
        {
            subwords.insert(*swit);
            if (swit->length() < 2) continue;
            vector<DecoderGraph::TriphoneNode> word_triphones;
            triphonize_subword(dg, *swit, word_triphones);
            add_triphones(triphone_nodes, word_triphones);
        }
    }

    triphones_to_states(dg, triphone_nodes, nodes);
    triphone_nodes.clear();
    prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    subwordgraphbuilder::create_crossword_network(dg, subwords, cw_nodes, fanout, fanin);

    cerr << endl << "cw size: " << cw_nodes.size() << endl;
    minimize_crossword_network(cw_nodes, fanout, fanin);
    cerr << "tied cw size: " << cw_nodes.size() << endl;

    subwordgraphbuilder::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin, false);
    connect_end_to_start_node(nodes);
    subwordgraphbuilder::connect_one_phone_subwords_from_start_to_cw(dg, subwords, nodes, fanout);
    subwordgraphbuilder::connect_one_phone_subwords_from_cw_to_end(dg, subwords, nodes, fanin);
    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false, false) );
}


void swgraphtest::SubwordGraphTest10(void)
{
    DecoderGraph dg;
    read_fixtures(dg);

    string segname = "data/segs.txt";
    map<string, vector<string> > word_segs;
    read_word_segmentations(dg, segname, word_segs);

    vector<DecoderGraph::Node> nodes(2);

    vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
    set<string> subwords;
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit)
    {
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit)
        {
            subwords.insert(*swit);
            if (swit->length() < 2) continue;
            vector<DecoderGraph::TriphoneNode> word_triphones;
            triphonize_subword(dg, *swit, word_triphones);
            add_triphones(triphone_nodes, word_triphones);
        }
    }

    triphones_to_states(dg, triphone_nodes, nodes);
    triphone_nodes.clear();
    prune_unreachable_nodes(nodes);

    set<node_idx_t> third_nodes;
    set_reverse_arcs(nodes);
    find_nodes_in_depth_reverse(nodes, third_nodes, 4);
    clear_reverse_arcs(nodes);
    for (auto nii=third_nodes.begin(); nii != third_nodes.end(); ++nii)
        nodes[*nii].flags |= NODE_LM_RIGHT_LIMIT;

    third_nodes.clear();
    find_nodes_in_depth(nodes, third_nodes, 4);
    for (auto nii=third_nodes.begin(); nii !=third_nodes.end(); ++nii)
        nodes[*nii].flags |= NODE_LM_LEFT_LIMIT;

    push_word_ids_right(nodes);
    tie_state_prefixes(nodes);
    push_word_ids_left(nodes);
    tie_state_suffixes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    subwordgraphbuilder::create_crossword_network(dg, subwords, cw_nodes, fanout, fanin);
    cerr << endl << "cw size: " << cw_nodes.size() << endl;
    minimize_crossword_network(cw_nodes, fanout, fanin);
    cerr << "tied cw size: " << cw_nodes.size() << endl;

    subwordgraphbuilder::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin, false);
    connect_end_to_start_node(nodes);
    subwordgraphbuilder::connect_one_phone_subwords_from_start_to_cw(dg, subwords, nodes, fanout);
    subwordgraphbuilder::connect_one_phone_subwords_from_cw_to_end(dg, subwords, nodes, fanin);
    prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT( assert_words(dg, nodes, word_segs, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, true, true, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, word_segs, false, false, false) );
}


//ofstream origoutf("acw.dot");
//print_dot_digraph(dg, nodes, origoutf, true);
//origoutf.close();
