#ifndef DECODER_GRAPH_CPPUNIT
#define DECODER_GRAPH_CPPUNIT

#define private public
#include "DecoderGraph.hh"
#undef private

#include <string>

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
    CPPUNIT_TEST (GraphTest12);
    CPPUNIT_TEST (GraphTest13);
    CPPUNIT_TEST (GraphTest14);
    CPPUNIT_TEST (GraphTest15);
    CPPUNIT_TEST (GraphTest16);
    CPPUNIT_TEST (GraphTest17);
    CPPUNIT_TEST (GraphTest18);
    CPPUNIT_TEST (GraphTest19);
    CPPUNIT_TEST (GraphTest20);
    CPPUNIT_TEST (GraphTest21);
    CPPUNIT_TEST (GraphTest22);
    CPPUNIT_TEST (GraphTest23);
    CPPUNIT_TEST (GraphTest24);
    CPPUNIT_TEST (GraphTest25);
    CPPUNIT_TEST (GraphTest26);
    CPPUNIT_TEST (GraphTest27);
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
    void GraphTest7(void);
    void GraphTest8(void);
    void GraphTest9(void);
    void GraphTest10(void);
    void GraphTest11(void);
    void GraphTest12(void);
    void GraphTest13(void);
    void GraphTest14(void);
    void GraphTest15(void);
    void GraphTest16(void);
    void GraphTest17(void);
    void GraphTest18(void);
    void GraphTest19(void);
    void GraphTest20(void);
    void GraphTest21(void);
    void GraphTest22(void);
    void GraphTest23(void);
    void GraphTest24(void);
    void GraphTest25(void);
    void GraphTest26(void);
    void GraphTest27(void);

private:

    std::string amname;
    std::string lexname;
    std::string segname;
    //std::vector<std::pair<std::string, std::vector<std::string> > > word_segs;
    std::map<std::string, std::vector<std::string> > word_segs;

    void read_fixtures(DecoderGraph &dg);

};

#endif
