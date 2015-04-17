#ifndef DECODER_NO_WB_SWGRAPH_CPPUNIT
#define DECODER_NO_WB_SWGRAPH_CPPUNIT

#define private public
#include "NoWBSubwordGraph.hh"
#undef private

#include <string>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class nowbswgraphtest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (nowbswgraphtest);
    CPPUNIT_TEST (NoWBSubwordGraphTest1);
    CPPUNIT_TEST_SUITE_END ();

public:
    void setUp(void);
    void tearDown(void);

protected:
    void NoWBSubwordGraphTest1(void);

private:

    std::string amname;
    std::string lexname;
    std::map<std::string, std::vector<std::string> > word_segs;
    std::set<std::string> subwords;

    void read_fixtures(NoWBSubwordGraph &dg,
                       std::string segfname);

};

#endif
