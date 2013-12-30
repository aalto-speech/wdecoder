#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "DecoderGraph.hh"

using namespace std;


void
DecoderGraph::read_noway_lexicon(string lexfname)
{
    ifstream lexf(lexfname);
    if (!lexf) throw string("Problem opening noway lexicon file.");

    string line;
    int linei = 1;
    while (getline(lexf, line)) {
        string unit;
        double prob = 1.0;
        vector<string> phones;

        string phone;
        stringstream ss(line);
        ss >> unit;
        while (ss >> phone) phones.push_back(phone);

        auto leftp = unit.find("(");
        if (leftp != string::npos) {
            auto rightp = unit.find(")");
            if (rightp == string::npos) throw string("Problem reading line " + linei);
            prob = stod(unit.substr(leftp+1, rightp-leftp-1));
            unit = unit.substr(0, leftp);
        }

        linei++;
    }


}


int
DecoderGraph::add_lm_unit(string unit)
{
    auto uit = m_unit_map.find(unit);
    if (uit != m_unit_map.end())
        return (*uit).second;

    int index = m_units.size();
    m_unit_map[unit] = index;
    m_units.push_back(unit);

    return index;
}


