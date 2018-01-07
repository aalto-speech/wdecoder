#include <boost/test/unit_test.hpp>
#include <set>

#include "DynamicBitset.hh"

using namespace std;


BOOST_AUTO_TEST_CASE(BitsetInitFalse)
{
    int bitsetSize = 100;
    DynamicBitset bs(bitsetSize, false);
    for (int i=0; i<bitsetSize; i++)
        BOOST_CHECK ( bs.getBit(i) == false );
}


BOOST_AUTO_TEST_CASE(BitsetInitTrue)
{
    int bitsetSize = 100;
    DynamicBitset bs(bitsetSize, true);
    for (int i=0; i<bitsetSize; i++)
        BOOST_CHECK ( bs.getBit(i) == true );
}


BOOST_AUTO_TEST_CASE(BitsetSetValuesToTrue)
{
    int bitsetSize = 100;
    DynamicBitset bs(bitsetSize, false);
    set<int> bitsToSet = { 4, 8, 9, 10, 50, 51, 52, 53 };

    for (auto bIt = bitsToSet.begin(); bIt != bitsToSet.end(); ++bIt)
        bs.setBit(*bIt, true);

    for (int i=0; i<bitsetSize; i++) {
        if (bitsToSet.find(i) == bitsToSet.end())
            BOOST_CHECK ( bs.getBit(i) == false );
        else
            BOOST_CHECK ( bs.getBit(i) == true );
    }
}


BOOST_AUTO_TEST_CASE(BitsetSetValuesToFalse)
{
    int bitsetSize = 100;
    DynamicBitset bs(bitsetSize, true);
    set<int> bitsToSet = { 4, 8, 9, 10, 50, 51, 52, 53 };

    for (auto bIt = bitsToSet.begin(); bIt != bitsToSet.end(); ++bIt)
        bs.setBit(*bIt, false);

    for (int i=0; i<bitsetSize; i++) {
        if (bitsToSet.find(i) == bitsToSet.end())
            BOOST_CHECK ( bs.getBit(i) == true );
        else
            BOOST_CHECK ( bs.getBit(i) == false );
    }
}
