#include <boost/test/unit_test.hpp>

#define private public
#include "Decoder.hh"
#include "ClassLookahead.hh"
#undef private

using namespace std;

// Check that the la states are set
BOOST_AUTO_TEST_CASE(ClassBigramLookaheadTest1)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.words.lex");
    d.read_dgraph("data/1k.words.graph");
    ClassBigramLookahead hypocla(
        d,
        "data/1k.words.exchange.2g.arpa.gz",
        "data/1k.words.exchange.cmemprobs.gz");
    BOOST_CHECK_EQUAL(hypocla.m_la_state_count, 896);
    for (unsigned int i=0; i<hypocla.m_node_la_states.size(); i++)
        BOOST_CHECK(hypocla.m_node_la_states[i] >= 0);
}


BOOST_AUTO_TEST_CASE(ClassBigramLookaheadTest2)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.words.lex");
    d.read_dgraph("data/1k.words.graph");
    DummyClassBigramLookahead refcla(
            d,
            "data/1k.words.exchange.2g.arpa.gz",
            "data/1k.words.exchange.cmemprobs.gz");

    ClassBigramLookahead hypocla(
            d,
            "data/1k.words.exchange.2g.arpa.gz",
            "data/1k.words.exchange.cmemprobs.gz");

    int sentence_begin_symbol_idx = d.m_text_unit_map.at("<s>");
    int sentence_end_symbol_idx = d.m_text_unit_map.at("</s>");

    cerr << "node count: " << d.m_nodes.size() << endl;
    cerr << "evaluating.." << endl;
    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        int curr_eval_ratio = d.m_nodes[i].flags & NODE_SILENCE ? 1 : 20;
        for (int w=0; w<(int)d.m_text_units.size(); w++) {
            if ((++idx % curr_eval_ratio != 0) && (w != sentence_begin_symbol_idx)) continue;
            if (w == sentence_end_symbol_idx) continue;
            float ref = refcla.get_lookahead_score(i, w);
            float hyp = hypocla.get_lookahead_score(i, w);
            BOOST_CHECK_CLOSE( ref, hyp, 0.001 );
        }
    }
}


// Graph without the sentence end symbol
BOOST_AUTO_TEST_CASE(ClassBigramLookaheadTest2NoSentenceEnd)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.words.lex");
    d.read_dgraph("data/1k.words.nose.graph");
    DummyClassBigramLookahead refcla(
            d,
            "data/1k.words.exchange.2g.arpa.gz",
            "data/1k.words.exchange.cmemprobs.gz");

    ClassBigramLookahead hypocla(
            d,
            "data/1k.words.exchange.2g.arpa.gz",
            "data/1k.words.exchange.cmemprobs.gz");

    int sentence_begin_symbol_idx = d.m_text_unit_map.at("<s>");
    int sentence_end_symbol_idx = d.m_text_unit_map.at("</s>");

    cerr << "node count: " << d.m_nodes.size() << endl;
    cerr << "evaluating.." << endl;
    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        int curr_eval_ratio = d.m_nodes[i].flags & NODE_SILENCE ? 1 : 20;
        for (int w=0; w<(int)d.m_text_units.size(); w++) {
            if ((++idx % curr_eval_ratio != 0) && (w != sentence_begin_symbol_idx)) continue;
            if (w == sentence_end_symbol_idx) continue;
            float ref = refcla.get_lookahead_score(i, w);
            float hyp = hypocla.get_lookahead_score(i, w);
            BOOST_CHECK_CLOSE( ref, hyp, 0.001 );
        }
    }
}


BOOST_AUTO_TEST_CASE(ClassBigramLookaheadTest2Quant)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.words.lex");
    d.read_dgraph("data/1k.words.graph");
    DummyClassBigramLookahead refcla(
            d,
            "data/1k.words.exchange.2g.arpa.gz",
            "data/1k.words.exchange.cmemprobs.gz");

    ClassBigramLookahead hypocla(
            d,
            "data/1k.words.exchange.2g.arpa.gz",
            "data/1k.words.exchange.cmemprobs.gz",
            true);

    int sentence_begin_symbol_idx = d.m_text_unit_map.at("<s>");
    int sentence_end_symbol_idx = d.m_text_unit_map.at("</s>");

    cerr << "node count: " << d.m_nodes.size() << endl;
    cerr << "evaluating.." << endl;
    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        int curr_eval_ratio = d.m_nodes[i].flags & NODE_SILENCE ? 1 : 20;
        for (int w=0; w<(int)d.m_text_units.size(); w++) {
            if ((++idx % curr_eval_ratio != 0) && (w != sentence_begin_symbol_idx)) continue;
            if (w == sentence_end_symbol_idx) continue;
            float ref = refcla.get_lookahead_score(i, w);
            float hyp = hypocla.get_lookahead_score(i, w);
            BOOST_CHECK_CLOSE( ref, hyp, 0.15 );
        }
    }
}


// Some words in the graph and lexicon not in the class n-gram look-ahead model
BOOST_AUTO_TEST_CASE(ClassBigramLookaheadTest3)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.words.lex");
    d.read_dgraph("data/1k.words.graph");
    DummyClassBigramLookahead refcla(
            d,
            "data/1k.words.exchange.2g.arpa.gz",
            "data/1k.words.exchange.cmemprobs.words_missing.gz");

    ClassBigramLookahead hypocla(
            d,
            "data/1k.words.exchange.2g.arpa.gz",
            "data/1k.words.exchange.cmemprobs.words_missing.gz");

    int sentence_begin_symbol_idx = d.m_text_unit_map.at("<s>");
    int sentence_end_symbol_idx = d.m_text_unit_map.at("</s>");

    cerr << "node count: " << d.m_nodes.size() << endl;
    cerr << "evaluating.." << endl;
    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        int curr_eval_ratio = d.m_nodes[i].flags & NODE_SILENCE ? 1 : 20;
        for (int w=0; w<(int)d.m_text_units.size(); w++) {
            if ((++idx % curr_eval_ratio != 0) && (w != sentence_begin_symbol_idx)) continue;
            if (w == sentence_end_symbol_idx) continue;
            if (refcla.m_class_la.m_class_membership_lookup[w].first == -1) continue;
            if (++idx % curr_eval_ratio != 0) continue;
            float ref = refcla.get_lookahead_score(i, w);
            float hyp = hypocla.get_lookahead_score(i, w);
            BOOST_CHECK_CLOSE( ref, hyp, 0.05 );
        }
    }
}


// Check that the la states are set
BOOST_AUTO_TEST_CASE(ClassBigramLookaheadTest20kWords)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/20k.words.lex");
    d.read_dgraph("data/20k.words.graph");
    ClassBigramLookahead hypocla(
        d,
        "data/20k.words.exchange.2g.arpa.gz",
        "data/20k.words.exchange.cmemprobs.gz");
}


// Check that the la states are set
BOOST_AUTO_TEST_CASE(ClassBigramLookaheadTest100kWords)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/100k.words.lex");
    d.read_dgraph("data/100k.words.graph");
    ClassBigramLookahead hypocla(
        d,
        "data/100k.words.exchange.2g.arpa.gz",
        "data/100k.words.exchange.cmemprobs.gz");
}
