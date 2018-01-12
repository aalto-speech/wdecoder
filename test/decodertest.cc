#include <boost/test/unit_test.hpp>

#define private public
#include "Decoder.hh"
#include "Lookahead.hh"
#undef private

using namespace std;

int eval_ratio = 50;
float tolerance = 0.0001;


BOOST_AUTO_TEST_CASE(BigramLookaheadTest1)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.words.lex");
    d.read_dgraph("data/1k.words.graph");
    DummyBigramLookahead refla(d, "data/1k.words.2gram.arpa");
    d.m_la = new FullTableBigramLookahead(d, "data/1k.words.2gram.arpa");

    cerr << "node count: " << d.m_nodes.size() << endl;
    cerr << "evaluating.." << endl;
    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        for (int w=0; w<(int)d.m_la->m_text_unit_id_to_la_ngram_symbol.size(); w++) {
            idx++;
            if (idx % eval_ratio != 0) continue;
            float ref = refla.get_lookahead_score(i, w);
            float hyp = d.m_la->get_lookahead_score(i, w);
            BOOST_CHECK_CLOSE( ref, hyp, tolerance );
        }
    }
}


BOOST_AUTO_TEST_CASE(BigramLookaheadTest2)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.words.lex");
    d.read_dgraph("data/1k.words.graph");
    DummyBigramLookahead refla(d, "data/1k.words.2gram.arpa");
    d.m_la = new PrecomputedFullTableBigramLookahead(d, "data/1k.words.2gram.arpa");

    cerr << "node count: " << d.m_nodes.size() << endl;
    cerr << "evaluating.." << endl;
    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        for (int w=0; w<(int)d.m_la->m_text_unit_id_to_la_ngram_symbol.size(); w++) {
            idx++;
            if (idx % eval_ratio != 0) continue;
            float ref = refla.get_lookahead_score(i, w);
            float hyp = d.m_la->get_lookahead_score(i, w);
            BOOST_CHECK_CLOSE( ref, hyp, tolerance );
        }
    }
}


BOOST_AUTO_TEST_CASE(BigramLookaheadTest2Quant)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.words.lex");
    d.read_dgraph("data/1k.words.graph");
    DummyBigramLookahead refla(d, "data/1k.words.2gram.arpa");
    d.m_la = new PrecomputedFullTableBigramLookahead(d, "data/1k.words.2gram.arpa", true);

    cerr << "node count: " << d.m_nodes.size() << endl;
    cerr << "evaluating.." << endl;
    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        for (int w=0; w<(int)d.m_la->m_text_unit_id_to_la_ngram_symbol.size(); w++) {
            idx++;
            if (idx % eval_ratio != 0) continue;
            float ref = refla.get_lookahead_score(i, w);
            float hyp = d.m_la->get_lookahead_score(i, w);
            BOOST_CHECK_CLOSE( ref, hyp, 0.05 );
        }
    }
}


BOOST_AUTO_TEST_CASE(BigramLookaheadTest3)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.words.lex");
    d.read_dgraph("data/1k.words.graph");
    LargeBigramLookahead *lbla = new LargeBigramLookahead(d, "data/1k.words.2gram.arpa");

    vector<set<int> > state_successors(lbla->m_lookahead_states.size());
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        int la_state = lbla->m_node_la_states[i];
        if (state_successors[la_state].size() == 0)
            lbla->find_successor_words(i, state_successors[la_state]);
    }

    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        int la_state = lbla->m_node_la_states[i];
        set<int> node_successors;
        lbla->find_successor_words(i, node_successors);
        BOOST_CHECK( state_successors[la_state] == node_successors );
    }
}


BOOST_AUTO_TEST_CASE(BigramLookaheadTest4)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.words.lex");
    d.read_dgraph("data/1k.words.graph");
    DummyBigramLookahead refla(d, "data/1k.words.2gram.arpa");
    d.m_la = new LargeBigramLookahead(d, "data/1k.words.2gram.arpa");

    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        for (int w=0; w<(int)d.m_la->m_text_unit_id_to_la_ngram_symbol.size(); w++) {
            idx++;
            if (idx % eval_ratio != 0) continue;
            float ref = refla.get_lookahead_score(i, w);
            float hyp = d.m_la->get_lookahead_score(i, w);
            BOOST_CHECK_CLOSE( ref, hyp, tolerance );
        }
    }
}


BOOST_AUTO_TEST_CASE(BigramLookaheadTest5)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.subwords.lex");
    d.read_dgraph("data/1k.subwords.sww.graph");
    DummyBigramLookahead refla(d, "data/1k.subwords.2g.arpa");
    d.m_la = new HybridBigramLookahead(d, "data/1k.subwords.2g.arpa");

    // Table nodes
    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        if (d.m_nodes[i].flags & NODE_BIGRAM_LA_TABLE) {
            for (int w=0; w<(int)d.m_la->m_text_unit_id_to_la_ngram_symbol.size(); w++) {
                idx++;
                if (idx % eval_ratio != 0) continue;
                float ref = refla.get_lookahead_score(i, w);
                float hyp = d.m_la->get_lookahead_score(i, w);
                BOOST_CHECK_CLOSE( ref, hyp, tolerance );
            }
        }
    }

    // Dictionary nodes
    vector<vector<Decoder::Arc> > reverse_arcs;
    refla.get_reverse_arcs(reverse_arcs);
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        if (!(d.m_nodes[i].flags & NODE_BIGRAM_LA_TABLE)) {
            set<int> pred_word_ids;
            d.m_la->find_predecessor_words(i, pred_word_ids, reverse_arcs);
            for (auto wit=pred_word_ids.begin(); wit != pred_word_ids.end(); ++wit) {
                float ref = refla.get_lookahead_score(i, *wit);
                float hyp = d.m_la->get_lookahead_score(i, *wit);
                BOOST_CHECK_CLOSE( ref, hyp, tolerance );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE(BigramLookaheadTest6)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/speecon_ml_gain3500_occ300_21.7.2011_22.ph");
    d.read_noway_lexicon("data/1k.subwords.lex");
    d.read_dgraph("data/1k.subwords.sww.graph");
    DummyBigramLookahead refla(d, "data/1k.subwords.2g.arpa");
    d.m_la = new PrecomputedHybridBigramLookahead(d, "data/1k.subwords.2g.arpa");

    // Table nodes
    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        if (d.m_nodes[i].flags & NODE_BIGRAM_LA_TABLE) {
            for (int w=0; w<(int)d.m_la->m_text_unit_id_to_la_ngram_symbol.size(); w++) {
                idx++;
                float ref = refla.get_lookahead_score(i, w);
                float hyp = d.m_la->get_lookahead_score(i, w);
                BOOST_CHECK_CLOSE( ref, hyp, tolerance );
            }
        }
    }

    // Dictionary nodes
    vector<vector<Decoder::Arc> > reverse_arcs;
    refla.get_reverse_arcs(reverse_arcs);
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        if (!(d.m_nodes[i].flags & NODE_BIGRAM_LA_TABLE)) {
            set<int> pred_word_ids;
            d.m_la->find_predecessor_words(i, pred_word_ids, reverse_arcs);
            for (auto wit=pred_word_ids.begin(); wit != pred_word_ids.end(); ++wit) {
                float ref = refla.get_lookahead_score(i, *wit);
                float hyp = d.m_la->get_lookahead_score(i, *wit);
                BOOST_CHECK_CLOSE( ref, hyp, tolerance );
            }
        }
    }
}


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
    BOOST_CHECK_EQUAL(hypocla.m_la_state_count, 350);
    for (int i=0; i<hypocla.m_node_la_states.size(); i++)
        BOOST_CHECK(hypocla.m_node_la_states[i] >= 0);
}


// Check that the la states are set
BOOST_AUTO_TEST_CASE(ClassBigramLookaheadTestBigYle)
{
    cerr << endl;
    Decoder d;
    d.read_phone_model("data/cbgla/speecon_mfcc_20.3.2012_20.ph");
    d.read_noway_lexicon("data/cbgla/lex");
    d.read_dgraph("data/cbgla/words.graph");
    ClassBigramLookahead hypocla(
        d,
        "data/cbgla/exchange.vkn.2g.D002_E004.arpa.gz",
        "data/cbgla/exchange.cmemprobs.gz");
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

    /*
    cerr << "node count: " << d.m_nodes.size() << endl;
    cerr << "evaluating.." << endl;
    int idx=0;
    for (int i=0; i<(int)d.m_nodes.size(); i++) {
        for (int w=0; w<(int)d.m_text_units.size(); w++) {
            if (++idx % eval_ratio != 0) continue;
            float ref = refcla.get_lookahead_score(i, w);
            cerr << "node: " << i << "\t"
                 << "word: " << w << "/" << d.m_text_units[w] << "\t"
                 << "ref lp: " << ref << endl;
            //float hyp = d.m_la->get_lookahead_score(i, w);
            //BOOST_CHECK_CLOSE( ref, hyp, tolerance );
        }
    }
    */
}
