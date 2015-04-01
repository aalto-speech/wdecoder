
#include "wgraphtest.hh"
#include "gutils.hh"
#include "WordGraphBuilder.hh"
#include "GraphBuilder.hh"


using namespace std;
using namespace gutils;


CPPUNIT_TEST_SUITE_REGISTRATION (wgraphtest);


void wgraphtest::setUp (void)
{
    amname = string("data/speecon_ml_gain3500_occ300_21.7.2011_22");
    lexname = string("data/1k.words.lex");
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
                            set<string> &words,
                            vector<DecoderGraph::Node> &nodes)
{
    for (auto wit = words.begin(); wit != words.end(); ++wit) {
        vector<TriphoneNode> word_triphones;
        triphonize_subword(dg, *wit, word_triphones);
        vector<DecoderGraph::Node> word_nodes;
        triphones_to_state_chain(dg, word_triphones, word_nodes);
        add_nodes_to_tree(dg, nodes, word_nodes);
    }
    lookahead_to_arcs(nodes);
    prune_unreachable_nodes(nodes);

    vector<DecoderGraph::Node> cw_nodes;
    map<string, int> fanout, fanin;
    wordgraphbuilder::create_crossword_network(dg, words, cw_nodes, fanout, fanin);
    cerr << "crossword network size: " << cw_nodes.size() << endl;
    minimize_crossword_network(cw_nodes, fanout, fanin);
    cerr << "tied crossword network size: " << cw_nodes.size() << endl;

    cerr << "Connecting crossword network.." << endl;
    graphbuilder::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
    connect_end_to_start_node(nodes);
    cerr << "number of hmm state nodes: " << reachable_graph_nodes(nodes) << endl;

    wordgraphbuilder::connect_one_phone_words_from_start_to_cw(dg, words, nodes, fanout);
    wordgraphbuilder::connect_one_phone_words_from_cw_to_end(dg, words, nodes, fanin);
}


// Test word graph construction with 1000 words
// No one phone words
void wgraphtest::WordGraphTest1(void)
{

    DecoderGraph dg;
    read_fixtures(dg);

    set<string> words;
    read_words(dg, "data/1k.words.txt", words);

    vector<DecoderGraph::Node> nodes(2);
    cerr << endl;
    make_graph(dg, words, nodes);
    cerr << "Number of lm id nodes: " << num_subword_states(nodes) << endl;

    CPPUNIT_ASSERT( assert_words(dg, nodes, words) );
    CPPUNIT_ASSERT( assert_only_words(dg, nodes, words) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, words, 20000) );
}


// Test word graph construction with 1000+ words
// Includes one phone words
void wgraphtest::WordGraphTest2(void)
{
    DecoderGraph dg;
    lexname = "data/500.words.1pwords.lex";
    read_fixtures(dg);

    set<string> words;
    read_words(dg, "data/500.words.1pwords.txt", words);

    vector<DecoderGraph::Node> nodes(2);
    cerr << endl;
    make_graph(dg, words, nodes);

    CPPUNIT_ASSERT_EQUAL( 524, num_subword_states(nodes) );
    CPPUNIT_ASSERT( assert_words(dg, nodes, words) );
    CPPUNIT_ASSERT( assert_only_words(dg, nodes, words) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, words) );
}

