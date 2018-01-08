#include <climits>
#include <string>
#include <iostream>

#include "defs.hh"
#include "QuantizedLogProb.hh"

using namespace std;

QuantizedLogProb::QuantizedLogProb(double minLogProb)
{
    setMinLogProb(minLogProb);
}

void
QuantizedLogProb::setMinLogProb(double minLogProb)
{
    m_minLogProb = minLogProb;
    m_quantizedLogProbs.clear();
    m_quantizedLogProbs.resize(USHRT_MAX+1);
    for (int i=0; i<(int)m_quantizedLogProbs.size(); i++)
        m_quantizedLogProbs[i] = double(i) / double(USHRT_MAX) * m_minLogProb;
}


unsigned short int
QuantizedLogProb::getQuantIndex(double logProb) {
    if (logProb < m_minLogProb)
        throw string("logProb " + double2str(logProb)
                + " was below the minimum value " + double2str(m_minLogProb));
    if (logProb > 0) throw string("invalid logProb " + double2str(logProb));
    return (unsigned short int)(round(logProb / m_minLogProb * float(USHRT_MAX)));
}


double
QuantizedLogProb::getQuantizedLogProb(unsigned short int quantIndex) {
    return m_quantizedLogProbs[quantIndex];
}
