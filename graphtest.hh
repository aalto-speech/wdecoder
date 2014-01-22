#ifndef DECODER_GRAPH_CPPUNIT
#define DECODER_GRAPH_CPPUNIT

#define private public
#include "DecoderGraph.hh"
#undef private

#include <deque>
#include <string>
#include <vector>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class graphtest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (graphtest);
    CPPUNIT_TEST (GraphTest1);
    CPPUNIT_TEST (GraphTest2);
    CPPUNIT_TEST (GraphTest3);
    CPPUNIT_TEST (GraphTest4);
    CPPUNIT_TEST (GraphTest5);
    CPPUNIT_TEST (GraphTest6);
    CPPUNIT_TEST (GraphTest7);
    CPPUNIT_TEST (GraphTest8);
    CPPUNIT_TEST (GraphTest9);
    CPPUNIT_TEST (GraphTest10);
    CPPUNIT_TEST (GraphTest11);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp(void);
        void tearDown(void);

        static void triphonize(std::string word,
                               std::vector<std::string> &triphones);
        static void triphonize(DecoderGraph &dg,
                               std::string word,
                               std::vector<std::string> &triphones);
        static void get_hmm_states(DecoderGraph &dg,
                                   const std::vector<std::string> &triphones,
                                   std::vector<int> &states);
        static bool assert_path(DecoderGraph &dg,
                                std::vector<DecoderGraph::Node> &nodes,
                                std::deque<int> states,
                                std::deque<std::string> subwords,
                                int node_idx);
        static bool assert_path(DecoderGraph &dg,
                                std::vector<DecoderGraph::Node> &nodes,
                                std::vector<std::string> &triphones,
                                std::vector<std::string> &subwords,
                                bool debug);
        static bool assert_transitions(DecoderGraph &dg,
                                       std::vector<DecoderGraph::Node> &nodes,
                                       bool debug);
        static bool assert_word_pair_crossword(DecoderGraph &dg,
                                               std::vector<DecoderGraph::Node> &nodes,
                                               std::string word1,
                                               std::string word2,
                                               bool debug);
        static bool assert_words(DecoderGraph &dg,
                                 std::vector<DecoderGraph::Node> &nodes,
                                 bool debug);
        static bool assert_word_pairs(DecoderGraph &dg,
                                      std::vector<DecoderGraph::Node> &nodes,
                                      bool debug);
        static bool assert_subword_id_positions(DecoderGraph &dg,
                                                std::vector<DecoderGraph::Node> &nodes,
                                                bool debug=false,
                                                int node_idx=DecoderGraph::START_NODE,
                                                int nodes_wo_branching=0);

    protected:
        void GraphTest1(void);
        void GraphTest2(void);
        void GraphTest3(void);
        void GraphTest4(void);
        void GraphTest5(void);
        void GraphTest6(void);
        void GraphTest7(void);
        void GraphTest8(void);
        void GraphTest9(void);
        void GraphTest10(void);
        void GraphTest11(void);

    private:

        std::string amname;
        std::string lexname;
        std::string segname;

        void read_fixtures(DecoderGraph &dg);

};

#endif
