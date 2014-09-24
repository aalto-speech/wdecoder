#ifndef DECODER_CPPUNIT
#define DECODER_CPPUNIT

#define private public
#include "Decoder.hh"
#include "Lookahead.hh"
#undef private

#include <deque>
#include <string>
#include <vector>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class decodertest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (decodertest);
    /*
    CPPUNIT_TEST (BigramLookaheadTest1);
    CPPUNIT_TEST (BigramLookaheadTest2);
    CPPUNIT_TEST (BigramLookaheadTest3);
    CPPUNIT_TEST (BigramLookaheadTest4);
    */
    CPPUNIT_TEST (BigramLookaheadTest5);
    CPPUNIT_TEST (BigramLookaheadTest6);
    CPPUNIT_TEST_SUITE_END ();

public:
    void setUp(void);
    void tearDown(void);

protected:
    void BigramLookaheadTest1(void);
    void BigramLookaheadTest2(void);
    void BigramLookaheadTest3(void);
    void BigramLookaheadTest4(void);
    void BigramLookaheadTest5(void);
    void BigramLookaheadTest6(void);

private:
};

#endif
