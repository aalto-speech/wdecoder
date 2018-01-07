#include <boost/test/unit_test.hpp>
#include <climits>
#include <iostream>

#define private public
#include "QuantizedLogProb.hh"
#undef private

using namespace std;

BOOST_AUTO_TEST_CASE(QuantizedLogProbInit)
{
    double minLogProb = -15.0;
    QuantizedLogProb qlp(minLogProb);
    BOOST_CHECK_EQUAL( qlp.m_minLogProb, minLogProb);
    BOOST_CHECK_EQUAL( qlp.m_quantizedLogProbs.size(), USHRT_MAX+1 );
    for (int i=1; i<(int)qlp.m_quantizedLogProbs.size(); i++)
        BOOST_CHECK( qlp.m_quantizedLogProbs[i] < qlp.m_quantizedLogProbs[i-1] );
    BOOST_CHECK_EQUAL( qlp.m_quantizedLogProbs[0], 0.0);
    BOOST_CHECK_EQUAL( qlp.m_quantizedLogProbs[USHRT_MAX], minLogProb);
}

BOOST_AUTO_TEST_CASE(QuantizedLogProbIndex)
{
    double minLogProb = -15.0;
    QuantizedLogProb qlp(minLogProb);
    double origLogProb = -8.0;
    unsigned short int qIdx = qlp.getQuantIndex(origLogProb);
    double qLogProb = qlp.getQuantizedLogProb(qIdx);
    BOOST_CHECK_CLOSE( origLogProb, qLogProb, 0.001);
    BOOST_CHECK_EQUAL( 34952, qIdx );
    BOOST_CHECK_EQUAL( qlp.getQuantIndex(0.0), 0 );
    BOOST_CHECK_EQUAL( qlp.getQuantIndex(minLogProb), USHRT_MAX );
}
