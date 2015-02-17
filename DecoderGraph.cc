#include <sstream>

#include "NowayHmmReader.hh"
#include "gutils.hh"

using namespace std;
using namespace gutils;


void
DecoderGraph::read_phone_model(string phnfname)
{
    ifstream phnf(phnfname);
    if (!phnf) throw string("Problem opening phone model.");

    int modelcount;
    NowayHmmReader::read(phnf, m_hmms, m_hmm_map, m_hmm_states, modelcount);

    set<string> sil_labels = { "_", "__", "_f", "_s" };
    m_states_per_phone = -1;
    for (int i=0; i<m_hmms.size(); i++) {
        Hmm &hmm = m_hmms[i];
        if (sil_labels.find(hmm.label) != sil_labels.end()) continue;
        if (m_states_per_phone == -1) m_states_per_phone = hmm.states.size();
        if (m_states_per_phone != hmm.states.size()) {
            cerr << "Varying amount of states for normal phones." << endl;
            exit(1);
        }
    }
    m_states_per_phone -= 2;
}


void
DecoderGraph::read_noway_lexicon(string lexfname)
{
    ifstream lexf(lexfname);
    if (!lexf) throw string("Problem opening noway lexicon file.");

    string line;
    int linei = 0;
    while (getline(lexf, line)) {
        linei++;
        string unit;
        vector<string> phones;

        string phone;
        stringstream ss(line);
        ss >> unit;
        while (ss >> phone) phones.push_back(phone);

        auto leftp = unit.find("(");
        if (leftp != string::npos) {
            auto rightp = unit.find(")");
            if (rightp == string::npos) throw string("Problem reading line " + linei);
            double prob = ::atof(unit.substr(leftp+1, rightp-leftp-1).c_str());
            unit = unit.substr(0, leftp);
        }

        bool problem_phone = false;
        for (auto pit = phones.begin(); pit != phones.end(); ++pit) {
            if (m_hmm_map.find(*pit) == m_hmm_map.end())
                problem_phone = true;
        }
        if (problem_phone) {
            cerr << "Unknown phone in line " << linei << ", " << line << endl;
            continue;
        }

        if (m_subword_map.find(unit) == m_subword_map.end()) {
            m_subwords.push_back(unit);
            m_subword_map[unit] = m_subwords.size()-1;
        }
        m_lexicon[unit] = phones;
    }
}



