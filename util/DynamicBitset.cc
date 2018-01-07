#include <climits>
#include <string>
#include <iostream>

#include "defs.hh"
#include "DynamicBitset.hh"

#define BITS_PER_CHAR 8

using namespace std;

DynamicBitset::DynamicBitset(
    int numBits,
    bool initValue)
    : numBits(numBits)
{
    data.resize((numBits / BITS_PER_CHAR) + 1, initValue ? UCHAR_MAX : 0);
}

bool
DynamicBitset::getBit(int idx)
{
    if (idx >= numBits)
        throw string("trying to get bit " + int2str(idx)
            + ", but the total number of bits is " + int2str(numBits));
    unsigned int byteIdx = idx / BITS_PER_CHAR;
    unsigned char bitMask = 1 << (idx % BITS_PER_CHAR);
    return data[byteIdx] & bitMask;
}

void
DynamicBitset::setBit(int idx, bool bitValue)
{
    if (idx >= numBits)
        throw string("trying to set bit " + int2str(idx)
            + ", but the total number of bits is " + int2str(numBits));
    unsigned int byteIdx = idx / BITS_PER_CHAR;
    unsigned char bitMask = 1 << (idx % BITS_PER_CHAR);
    if ((data[byteIdx] & bitMask) != bitValue)
        data[byteIdx] ^= bitMask;
}
