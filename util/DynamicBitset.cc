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
    : m_numBits(numBits)
{
    m_data.resize((numBits / BITS_PER_CHAR) + 1, initValue ? UCHAR_MAX : 0);
}


void
DynamicBitset::resize(
    int numBits,
    bool initValue)
{
    m_numBits = numBits;
    m_data.clear();
    m_data.resize((numBits / BITS_PER_CHAR) + 1, initValue ? UCHAR_MAX : 0);
}


bool
DynamicBitset::getBit(int idx)
{
    if (idx >= m_numBits)
        throw string("trying to get bit " + int2str(idx)
            + ", but the total number of bits is " + int2str(m_numBits));
    unsigned int byteIdx = idx / BITS_PER_CHAR;
    unsigned char bitMask = 1 << (idx % BITS_PER_CHAR);
    return m_data[byteIdx] & bitMask;
}

void
DynamicBitset::setBit(int idx, bool bitValue)
{
    if (idx >= m_numBits)
        throw string("trying to set bit " + int2str(idx)
            + ", but the total number of bits is " + int2str(m_numBits));
    unsigned int byteIdx = idx / BITS_PER_CHAR;
    unsigned char bitMask = 1 << (idx % BITS_PER_CHAR);
    if ((m_data[byteIdx] & bitMask) != bitValue)
        m_data[byteIdx] ^= bitMask;
}
