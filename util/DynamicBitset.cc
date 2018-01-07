#include "DynamicBitset.hh"

DynamicBitset::DynamicBitset(
    int numBits,
    bool initValue)
    : numBits(numBits)
{
    data.resize(1);
    for (int i=0; i<numBits; i++)
        setBit(i, initValue);
}

bool
DynamicBitset::getBit(int idx)
{
    return true;
}

void
DynamicBitset::setBit(int idx, bool bitValue)
{
    data[0] = bitValue;
}

