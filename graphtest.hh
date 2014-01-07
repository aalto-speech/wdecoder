#ifndef DECODER_GRAPH_CPPUNIT
#define DECODER_GRAPH_CPPUNIT

#define private public
#include "DecoderGraph.hh"
#undef private

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class graphtest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (graphtest);
    CPPUNIT_TEST (GraphTest1);
    CPPUNIT_TEST (GraphTest2);
    CPPUNIT_TEST_SUITE_END ();

    public:
        void setUp(void);
        void tearDown(void);

    protected:
        void GraphTest1(void);
        void GraphTest2(void);
};

#endif
