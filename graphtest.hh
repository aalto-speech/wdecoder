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
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp(void);
        void tearDown(void);

    protected:
        void GraphTest1(void);
        void GraphTest2(void);
        void GraphTest3(void);
        void GraphTest4(void);
        void GraphTest5(void);
        void GraphTest6(void);

    private:
        void triphonize(std::string word,
                        std::vector<std::string> &triphones);
        void get_hmm_states(DecoderGraph &dg,
                            const std::vector<std::string> &triphones,
                            std::vector<int> &states);
        bool assert_path(DecoderGraph &dg,
                         std::vector<DecoderGraph::Node> &nodes,
                         std::deque<int> states,
                         std::deque<std::string> subwords,
                         int node_idx);
        bool assert_path(DecoderGraph &dg,
                         std::vector<DecoderGraph::Node> &nodes,
                         std::vector<std::string> &triphones,
                         std::vector<std::string> &subwords,
                         bool debug);
        bool assert_transitions(DecoderGraph &dg,
                                std::vector<DecoderGraph::Node> &nodes,
                                bool debug);
};

#endif
