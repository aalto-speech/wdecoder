#ifndef DYNAMIC_BITSET
#define DYNAMIC_BITSET

#include <vector>

class DynamicBitset {
public:
    DynamicBitset(int numBits = 0, bool initValue = false);
    void resize(int numBits, bool initValue = false);
    int size() { return m_numBits; }
    bool getBit(int idx);
    void setBit(int idx, bool bitValue);

private:
    int m_numBits;
    std::vector<unsigned char> m_data;
};

#endif
