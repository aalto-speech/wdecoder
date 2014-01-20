#include <string>
#include <iostream>

#include "graphtest.hh"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace std;


CPPUNIT_TEST_SUITE_REGISTRATION (graphtest);

void graphtest :: setUp (void)
{
}

void graphtest :: tearDown (void)
{
}


void graphtest :: triphonize(string word,
                             vector<string> &triphones)
{
    string tword = "_" + word + "_";
    triphones.clear();
    for (int i=1; i<tword.length()-1; i++) {
        stringstream tstring;
        tstring << tword[i-1] << "-" << tword[i] << "+" << tword[i+1];
        triphones.push_back(tstring.str());
    }
}


void graphtest :: get_hmm_states(DecoderGraph &dg,
                                 const vector<string> &triphones,
                                 vector<int> &states)
{
    for (auto tit = triphones.begin(); tit !=triphones.end(); ++tit) {
        int hmm_index = dg.m_hmm_map[*tit];
        Hmm &hmm = dg.m_hmms[hmm_index];
        for (int sidx = 2; sidx < hmm.states.size(); ++sidx)
            states.push_back(hmm.states[sidx].model);
    }
}


bool
graphtest :: assert_path(DecoderGraph &dg,
                         vector<DecoderGraph::Node> &nodes,
                         deque<int> states,
                         deque<string> subwords,
                         int node_idx)
{
    DecoderGraph::Node &curr_node = nodes[node_idx];

    if (states.size() == 0) {
        if (subwords.size() > 0) return false;
        else return true;
    }

    if (curr_node.hmm_state != -1) {
        if (states.back() != curr_node.hmm_state) {
            return false;
        }
        else states.pop_back();
    }

    if (curr_node.word_id != -1) {
        if (subwords.size() == 0) return false;
        if (subwords.back() != dg.m_units[curr_node.word_id]) return false;
        else subwords.pop_back();
    }

    for (auto ait = curr_node.arcs.begin(); ait != curr_node.arcs.end(); ++ait) {
        bool retval = assert_path(dg, nodes, states, subwords, ait->target_node);
        if (retval) return true;
    }

    return false;
}


bool
graphtest :: assert_path(DecoderGraph &dg,
                         vector<DecoderGraph::Node> &nodes,
                         vector<string> &triphones,
                         vector<string> &subwords,
                         bool debug)
{
    deque<int> dstates;
    deque<string> dwords;

    for (auto tit = triphones.begin(); tit != triphones.end(); ++tit) {
        int hmm_index = dg.m_hmm_map[*tit];
        Hmm &hmm = dg.m_hmms[hmm_index];
        for (auto sit = hmm.states.begin(); sit != hmm.states.end(); ++sit)
            if (sit->model >= 0) dstates.push_front(sit->model);
    }
    for (auto wit = subwords.begin(); wit != subwords.end(); ++wit)
        dwords.push_front(*wit);

    if (debug) {
        cerr << "expecting hmm states: " << endl;
        for (auto it = dstates.rbegin(); it != dstates.rend(); ++it)
            cerr << " " << *it;
        cerr << endl;
        cerr << "expecting subwords: " << endl;
        for (auto it = subwords.begin(); it != subwords.end(); ++it)
            cerr << " " << *it;
        cerr << endl;
    }

    return assert_path(dg, nodes, dstates, dwords, DecoderGraph::START_NODE);
}


bool
graphtest :: assert_word_pair_crossword(DecoderGraph &dg,
                                        vector<DecoderGraph::Node> &nodes,
                                        string word1,
                                        string word2,
                                        bool debug)
{
    if (dg.m_lexicon.find(word1) == dg.m_lexicon.end()) return false;
    if (dg.m_lexicon.find(word2) == dg.m_lexicon.end()) return false;

    char first_last = dg.m_lexicon[word1].back()[2];
    char second_first = dg.m_lexicon[word2][0][2];

    vector<string> triphones;
    for (int i = 0; i < dg.m_lexicon[word1].size()-1; ++i)
        triphones.push_back(dg.m_lexicon[word1][i]);
    string last_triphone = dg.m_lexicon[word1][dg.m_lexicon[word1].size()-1].substr(0,4) + second_first;
    triphones.push_back(last_triphone);

    string first_triphone = first_last + dg.m_lexicon[word2][0].substr(1,4);
    triphones.push_back(first_triphone);
    for (int i = 1; i < dg.m_lexicon[word2].size(); ++i)
        triphones.push_back(dg.m_lexicon[word2][i]);

    vector<string> subwords;
    for (auto swit = dg.m_word_segs[word1].begin(); swit != dg.m_word_segs[word1].end(); ++swit)
        subwords.push_back(*swit);
    for (auto swit = dg.m_word_segs[word2].begin(); swit != dg.m_word_segs[word2].end(); ++swit)
        subwords.push_back(*swit);

    return assert_path(dg, nodes, triphones, subwords, debug);
}


bool
graphtest :: assert_words(DecoderGraph &dg,
                          std::vector<DecoderGraph::Node> &nodes,
                          bool debug)
{
    for (auto sit=dg.m_word_segs.begin(); sit!=dg.m_word_segs.end(); ++sit) {
        vector<string> triphones;
        triphonize(sit->first, triphones);
        bool result = assert_path(dg, nodes, triphones, sit->second, false);
        if (!result) return false;
    }
    return true;
}


bool
graphtest :: assert_word_pairs(DecoderGraph &dg,
                               std::vector<DecoderGraph::Node> &nodes,
                               bool debug)
{
    return false;
}


bool
graphtest :: assert_transitions(DecoderGraph &dg,
                                std::vector<DecoderGraph::Node> &nodes,
                                bool debug)
{
    for (int node_idx = 0; node_idx < nodes.size(); ++node_idx) {
        if (node_idx == DecoderGraph::END_NODE) continue;
        DecoderGraph::Node &node = nodes[node_idx];
        if (!node.arcs.size()) {
            if (debug) cerr << "Node " << node_idx << " has no transitions" << endl;
            return false;
        }

        if (node.hmm_state == -1) {
            for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
                if (ait->log_prob != 0.0) {
                    if (debug) cerr << "Node " << node_idx << " non hmm state out transition lp: " << ait->log_prob << endl;
                    return false;
                }
                if (ait->target_node == node_idx) {
                    if (debug) cerr << "Node " << node_idx << " self-transition in non-hmm-node " << endl;
                    return false;
                }
            }
            continue;
        }

        HmmState &state = dg.m_hmm_states[node.hmm_state];
        bool self_transition = false;
        bool out_transition = false;
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
            if (ait->target_node == node_idx) {
                self_transition = true;
                if (ait->log_prob != state.transitions[0].log_prob) {
                    if (debug) cerr << "Node " << node_idx << " invalid self-transition prob" << endl;
                    return false;
                }
            }
            else {
                out_transition = true;
                if (ait->log_prob != state.transitions[1].log_prob) {
                    if (debug) cerr << "Node " << node_idx << " invalid out-transition prob" << endl;
                    return false;
                }
            }
        }
        if (!self_transition) {
            if (debug) cerr << "Node " << node_idx << " has no self-transition" << endl;
            return false;
        }
        if (!out_transition) {
            if (debug) cerr << "Node " << node_idx << " has no out-transition" << endl;
            return false;
        }
    }
    return true;
}


bool
graphtest :: assert_subword_id_positions(DecoderGraph &dg,
                                         std::vector<DecoderGraph::Node> &nodes,
                                         bool debug,
                                         int node_idx,
                                         int nodes_wo_branching)
{
    if (node_idx == DecoderGraph::END_NODE) return true;

    DecoderGraph::Node &node = nodes[node_idx];

    if (node.word_id != -1) {
        if (nodes_wo_branching > 0) {
            if (debug) cerr << endl << "problem in node " << node_idx
                << ",subword: " << dg.m_units[node.word_id] << " nodes_wo_branching: " << nodes_wo_branching <<endl;
            return false;
        }
        else nodes_wo_branching = 0;
    }
    else {
        if (node.arcs.size() == 1) nodes_wo_branching += 1;
        else nodes_wo_branching = 0;
    }

    for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait) {
        if (ait->target_node == node_idx) throw string("Call before setting self-transitions.");
        bool retval = assert_subword_id_positions(dg, nodes, debug, ait->target_node, nodes_wo_branching);
        if (!retval) return false;
    }
    return true;
}


// Verify that models are correctly loaded
// Test constructing the initial word graph on subword level
void graphtest :: GraphTest1(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    CPPUNIT_ASSERT_EQUAL( 35003, (int)dg.m_units.size() );
    CPPUNIT_ASSERT_EQUAL( 35003, (int)dg.m_unit_map.size() ); // ?
    CPPUNIT_ASSERT_EQUAL( 13252, (int)dg.m_hmms.size() );
    CPPUNIT_ASSERT_EQUAL( 13252, (int)dg.m_hmm_map.size() );
    CPPUNIT_ASSERT_EQUAL( 1170, (int)dg.m_hmm_states.size() );
    CPPUNIT_ASSERT_EQUAL( 9, (int)dg.m_word_segs.size() );

    vector<DecoderGraph::SubwordNode> nodes;
    dg.create_word_graph(nodes);
    CPPUNIT_ASSERT_EQUAL( 12, dg.reachable_word_graph_nodes(nodes) );
}


// Test tying word suffixes
void graphtest :: GraphTest2(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    vector<DecoderGraph::SubwordNode> nodes;
    dg.create_word_graph(nodes);
    CPPUNIT_ASSERT_EQUAL( 12, dg.reachable_word_graph_nodes(nodes) );
    dg.tie_word_graph_suffixes(nodes);
    CPPUNIT_ASSERT_EQUAL( 11, dg.reachable_word_graph_nodes(nodes) );
}


// Test expanding the subwords to triphones
void graphtest :: GraphTest3(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_word_graph_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes(2);
    dg.expand_subword_nodes(swnodes, nodes, 0);
    CPPUNIT_ASSERT_EQUAL( 147, (int)nodes.size() );
    CPPUNIT_ASSERT_EQUAL( 147, (int)dg.reachable_graph_nodes(nodes) );
    // FIXME
    // CPPUNIT_ASSERT_EQUAL( 9, dg.num_subword_states(nodes) );

    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
}


// Test tying state chain prefixes
void graphtest :: GraphTest4(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_word_graph_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes(2);
    dg.expand_subword_nodes(swnodes, nodes, 0);
    CPPUNIT_ASSERT_EQUAL( 147, (int)dg.reachable_graph_nodes(nodes) );
    dg.tie_state_prefixes(nodes, 0, DecoderGraph::START_NODE);
    CPPUNIT_ASSERT_EQUAL( 121, (int)dg.reachable_graph_nodes(nodes) );

    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
}


// Verify adding self transitions and transition probabilities to states
void graphtest :: GraphTest5(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_word_graph_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes(2);
    dg.expand_subword_nodes(swnodes, nodes, 0);
    CPPUNIT_ASSERT_EQUAL( 147, (int)dg.reachable_graph_nodes(nodes) );
    dg.tie_state_prefixes(nodes, 0, DecoderGraph::START_NODE);
    CPPUNIT_ASSERT_EQUAL( 121, (int)dg.reachable_graph_nodes(nodes) );

    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );

    dg.add_hmm_self_transitions(nodes);
    dg.set_hmm_transition_probs(nodes);
    CPPUNIT_ASSERT( assert_transitions(dg, nodes, true) );
}


// Test pruning non-reachable nodes and reindexing nodes
void graphtest :: GraphTest6(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_word_graph_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes(2);
    dg.expand_subword_nodes(swnodes, nodes, 0);
    CPPUNIT_ASSERT_EQUAL( 147, (int)dg.reachable_graph_nodes(nodes) );
    dg.tie_state_prefixes(nodes, 0, DecoderGraph::START_NODE);
    CPPUNIT_ASSERT_EQUAL( 121, (int)dg.reachable_graph_nodes(nodes) );

    dg.prune_unreachable_nodes(nodes);
    CPPUNIT_ASSERT_EQUAL( 121, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 121, (int)nodes.size() );

    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
}


// Test pushing subword ids to the leftmost possible position
void graphtest :: GraphTest7(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";
    //string segname = "data/iter111_35000.train.segs2";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_word_graph_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes(2);
    dg.expand_subword_nodes(swnodes, nodes, 0);
    CPPUNIT_ASSERT_EQUAL( 147, (int)dg.reachable_graph_nodes(nodes) );
    dg.tie_state_prefixes(nodes, 0, DecoderGraph::START_NODE);
    CPPUNIT_ASSERT_EQUAL( 121, (int)dg.reachable_graph_nodes(nodes) );

    dg.prune_unreachable_nodes(nodes);
    dg.push_word_ids_left(nodes, false);
    dg.prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT_EQUAL( 121, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 121, (int)nodes.size() );

    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_subword_id_positions(dg, nodes, false) );
}


// Test cross-word network creation
void graphtest :: GraphTest8(void)
{
    string amname = "data/speecon_ml_gain3500_occ300_21.7.2011_22";
    string lexname = "data/lex";
    string segname = "data/segs.txt";
    //string segname = "data/iter111_35000.train.segs2";

    DecoderGraph dg;
    dg.read_phone_model(amname + ".ph");
    dg.read_duration_model(amname + ".dur");
    dg.read_noway_lexicon(lexname);
    dg.read_word_segmentations(segname);

    vector<DecoderGraph::SubwordNode> swnodes;
    dg.create_word_graph(swnodes);
    dg.tie_word_graph_suffixes(swnodes);
    vector<DecoderGraph::Node> nodes(2);
    dg.expand_subword_nodes(swnodes, nodes, 0);
    CPPUNIT_ASSERT_EQUAL( 147, (int)dg.reachable_graph_nodes(nodes) );
    dg.tie_state_prefixes(nodes, 0, DecoderGraph::START_NODE);
    CPPUNIT_ASSERT_EQUAL( 121, (int)dg.reachable_graph_nodes(nodes) );

    dg.prune_unreachable_nodes(nodes);
    dg.push_word_ids_left(nodes, false);
    dg.prune_unreachable_nodes(nodes);

    CPPUNIT_ASSERT_EQUAL( 121, (int)dg.reachable_graph_nodes(nodes) );
    CPPUNIT_ASSERT_EQUAL( 121, (int)nodes.size() );

    CPPUNIT_ASSERT( assert_subword_id_positions(dg, nodes, false) );

    CPPUNIT_ASSERT( assert_words(dg, nodes, false) );
    CPPUNIT_ASSERT( assert_word_pairs(dg, nodes, false) );
}

