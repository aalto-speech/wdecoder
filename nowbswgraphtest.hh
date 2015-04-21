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
    CPPUNIT_TEST (NoWBSubwordGraphTest2);
    CPPUNIT_TEST (NoWBSubwordGraphTest3);
    CPPUNIT_TEST (NoWBSubwordGraphTest4);
    CPPUNIT_TEST (NoWBSubwordGraphTest5);
    CPPUNIT_TEST_SUITE_END ();

public:
    void setUp(void);
    void tearDown(void);

protected:
    void NoWBSubwordGraphTest1(void);
    void NoWBSubwordGraphTest2(void);
    void NoWBSubwordGraphTest3(void);
    void NoWBSubwordGraphTest4(void);
    void NoWBSubwordGraphTest5(void);

private:

    std::string amname;
    std::string lexname;
    std::set<std::string> word_start_subwords;
    std::set<std::string> subwords;

    void read_fixtures(NoWBSubwordGraph &dg);
    void construct_words(const std::set<std::string> &word_start_subwords,
                         const std::set<std::string> &subwords,
                         std::map<std::string, std::vector<std::string> > &word_segs);
    void construct_complex_words(const std::set<std::string> &word_start_subwords,
                                 const std::set<std::string> &subwords,
                                 std::map<std::string, std::vector<std::string> > &word_segs);
};

#endif
