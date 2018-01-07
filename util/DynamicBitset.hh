#ifndef DYNAMIC_BITSET
#define DYNAMIC_BITSET

#include <vector>

class DynamicBitset {
public:
    DynamicBitset(int numBits, bool initValue = false);
    bool getBit(int idx);
    void setBit(int idx, bool bitValue);

private:
    int numBits;
    std::vector<unsigned char> data;
};

#endif
