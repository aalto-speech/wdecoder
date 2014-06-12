#ifndef DECODER_CPPUNIT
#define DECODER_CPPUNIT

#define private public
#include "Decoder.hh"
#undef private

#include <deque>
#include <string>
#include <vector>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class decodertest : public CPPUNIT_NS :: TestFixture
{
    CPPUNIT_TEST_SUITE (decodertest);
    CPPUNIT_TEST (DecoderTest1);
    CPPUNIT_TEST_SUITE_END ();

public:
    void setUp(void);
    void tearDown(void);

protected:
    void DecoderTest1(void);

private:
    Decoder d;
};

#endif
