#ifndef DECODER_SWGRAPH_CPPUNIT
#define DECODER_SWGRAPH_CPPUNIT

#define private public
#include "SubwordGraphBuilder.hh"
#undef private

#include <string>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class swgraphtest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (swgraphtest);
    CPPUNIT_TEST (SubwordGraphTest1);
    CPPUNIT_TEST (SubwordGraphTest2);
    CPPUNIT_TEST (SubwordGraphTest3);
    CPPUNIT_TEST (SubwordGraphTest4);
    CPPUNIT_TEST (SubwordGraphTest5);
    CPPUNIT_TEST (SubwordGraphTest6);
    CPPUNIT_TEST (SubwordGraphTest7);
    CPPUNIT_TEST (SubwordGraphTest8);
    CPPUNIT_TEST_SUITE_END ();

public:
    void setUp(void);
    void tearDown(void);

protected:
    void SubwordGraphTest1(void);
    void SubwordGraphTest2(void);
    void SubwordGraphTest3(void);
    void SubwordGraphTest4(void);
    void SubwordGraphTest5(void);
    void SubwordGraphTest6(void);
    void SubwordGraphTest7(void);
    void SubwordGraphTest8(void);

private:

    std::string amname;
    std::string lexname;
    std::map<std::string, std::vector<std::string> > word_segs;
    std::set<std::string> subwords;

    void read_fixtures(SubwordGraph &dg,
                       std::string segfname);

};

#endif
