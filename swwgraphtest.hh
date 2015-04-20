#ifndef SWWGRAPH_CPPUNIT
#define SWWGRAPH_CPPUNIT

#define private public
#include "SWWGraph.hh"
#undef private

#include <string>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class swwgraphtest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (swwgraphtest);
    CPPUNIT_TEST (SWWGraphTest1);
    CPPUNIT_TEST (SWWGraphTest2);
    CPPUNIT_TEST (SWWGraphTest3);
    CPPUNIT_TEST (SWWGraphTest4);
    CPPUNIT_TEST (SWWGraphTest5);
    CPPUNIT_TEST (SWWGraphTest6);
    CPPUNIT_TEST (SWWGraphTest7);
    CPPUNIT_TEST (SWWGraphTest8);
    CPPUNIT_TEST (SWWGraphTest9);
    CPPUNIT_TEST (SWWGraphTest10);
    CPPUNIT_TEST (SWWGraphTest11);
    CPPUNIT_TEST (SWWGraphTest12);
    CPPUNIT_TEST (SWWGraphTest13);
    CPPUNIT_TEST (SWWGraphTest14);
    CPPUNIT_TEST (SWWGraphTest15);
    CPPUNIT_TEST (SWWGraphTest16);
    CPPUNIT_TEST (SWWGraphTest17);
    CPPUNIT_TEST (SWWGraphTest18);
    CPPUNIT_TEST (SWWGraphTest19);
    CPPUNIT_TEST (SWWGraphTest20);
    CPPUNIT_TEST (SWWGraphTest21);
    CPPUNIT_TEST (SWWGraphTest22);
    CPPUNIT_TEST (SWWGraphTest23);
    CPPUNIT_TEST (SWWGraphTest24);
    CPPUNIT_TEST_SUITE_END ();

public:
    void setUp(void);
    void tearDown(void);

protected:
    void SWWGraphTest1(void);
    void SWWGraphTest2(void);
    void SWWGraphTest3(void);
    void SWWGraphTest4(void);
    void SWWGraphTest5(void);
    void SWWGraphTest6(void);
    void SWWGraphTest7(void);
    void SWWGraphTest8(void);
    void SWWGraphTest9(void);
    void SWWGraphTest10(void);
    void SWWGraphTest11(void);
    void SWWGraphTest12(void);
    void SWWGraphTest13(void);
    void SWWGraphTest14(void);
    void SWWGraphTest15(void);
    void SWWGraphTest16(void);
    void SWWGraphTest17(void);
    void SWWGraphTest18(void);
    void SWWGraphTest19(void);
    void SWWGraphTest20(void);
    void SWWGraphTest21(void);
    void SWWGraphTest22(void);
    void SWWGraphTest23(void);
    void SWWGraphTest24(void);

private:

    std::string amname;
    std::string lexname;
    std::string segname;
    std::map<std::string, std::vector<std::string> > word_segs;

    void read_fixtures(SWWGraph &swwg);

};

#endif
