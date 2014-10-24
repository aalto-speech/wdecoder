
#include "decodertest.hh"


using namespace std;

int eval_ratio = 50;


CPPUNIT_TEST_SUITE_REGISTRATION (decodertest);


void decodertest::setUp (void)
{
}


void decodertest::tearDown (void)
{
}


void decodertest::BigramLookaheadTest1(void)
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
        for (int w=0; w<(int)d.m_la->m_subword_id_to_la_ngram_symbol.size(); w++) {
            idx++;
            if (idx % eval_ratio != 0) continue;
            float ref = refla.get_lookahead_score(i, w);
            float hyp = d.m_la->get_lookahead_score(i, w);
            CPPUNIT_ASSERT_EQUAL( ref, hyp );
        }
    }
}


void decodertest::BigramLookaheadTest2(void)
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
        for (int w=0; w<(int)d.m_la->m_subword_id_to_la_ngram_symbol.size(); w++) {
            idx++;
            if (idx % eval_ratio != 0) continue;
            float ref = refla.get_lookahead_score(i, w);
            float hyp = d.m_la->get_lookahead_score(i, w);
            CPPUNIT_ASSERT_EQUAL( ref, hyp );
        }
    }
}


void decodertest::BigramLookaheadTest3(void)
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
        CPPUNIT_ASSERT( state_successors[la_state] == node_successors );
    }
}


void decodertest::BigramLookaheadTest4(void)
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
        for (int w=0; w<(int)d.m_la->m_subword_id_to_la_ngram_symbol.size(); w++) {
            idx++;
            if (idx % eval_ratio != 0) continue;
            float ref = refla.get_lookahead_score(i, w);
            float hyp = d.m_la->get_lookahead_score(i, w);
            CPPUNIT_ASSERT_EQUAL( ref, hyp );
        }
    }
}


void decodertest::BigramLookaheadTest5(void)
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
            for (int w=0; w<(int)d.m_la->m_subword_id_to_la_ngram_symbol.size(); w++) {
                idx++;
                if (idx % eval_ratio != 0) continue;
                float ref = refla.get_lookahead_score(i, w);
                float hyp = d.m_la->get_lookahead_score(i, w);
                CPPUNIT_ASSERT_EQUAL( ref, hyp );
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
                CPPUNIT_ASSERT_EQUAL( ref, hyp );
            }
        }
    }
}


void decodertest::BigramLookaheadTest6(void)
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
            for (int w=0; w<(int)d.m_la->m_subword_id_to_la_ngram_symbol.size(); w++) {
                idx++;
                float ref = refla.get_lookahead_score(i, w);
                float hyp = d.m_la->get_lookahead_score(i, w);
                CPPUNIT_ASSERT_EQUAL( ref, hyp );
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
                CPPUNIT_ASSERT_EQUAL( ref, hyp );
            }
        }
    }
}

