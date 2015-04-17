#ifndef DECODER_WGRAPH_CPPUNIT
#define DECODER_WGRAPH_CPPUNIT

#define private public
#include "WordGraph.hh"
#undef private

#include <set>
#include <string>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class wgraphtest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (wgraphtest);
    CPPUNIT_TEST (WordGraphTest1);
    CPPUNIT_TEST (WordGraphTest2);
    CPPUNIT_TEST_SUITE_END ();

public:
    void setUp(void);
    void tearDown(void);

protected:
    void WordGraphTest1(void);
    void WordGraphTest2(void);

private:

    std::string amname;
    std::string lexname;
    std::string segname;

    void read_fixtures(WordGraph &dg);

};

#endif
