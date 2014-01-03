#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "NowayHmmReader.hh"
#include "DecoderGraph.hh"

using namespace std;



void
DecoderGraph::read_phone_model(string phnfname)
{
    ifstream phnf(phnfname);
    if (!phnf) throw string("Problem opening phone model.");

    NowayHmmReader::read(phnf, m_hmms, m_hmm_map, m_num_models);

    cerr << "m_hmms size: " << m_hmms.size() << endl;
    cerr << "m_hmm_map size: " << m_hmm_map.size() << endl;
    cerr << "m_num_models: " << m_num_models << endl;
}


void
DecoderGraph::read_duration_model(string durfname)
{
    ifstream durf(durfname);
    if (!durf) throw string("Problem opening duration model.");

    NowayHmmReader::read_durations(durf, m_hmms);
}


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


