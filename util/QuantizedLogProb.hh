#ifndef QUANTIZED_LOG_PROB
#define QUANTIZED_LOG_PROB

#include <vector>

class QuantizedLogProb {
public:
    QuantizedLogProb(double minLogProb = -100.0);
    void setMinLogProb(double minLogProb);
    unsigned short int getQuantIndex(double logProb);
    double getQuantizedLogProb(unsigned short int quantIndex);

private:
    double m_minLogProb;
    std::vector<double> m_quantizedLogProbs;
};

#endif
