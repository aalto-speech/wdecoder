#include <sstream>

#include "DecoderGraph.hh"
#include "conf.hh"

using namespace std;


void
fix_noway_lexicon(DecoderGraph &dg,
                  string lexfname,
                  string fixlexfname)
{
    ifstream inlexf(lexfname);
    if (!inlexf) throw string("Problem opening noway lexicon file.");

    ofstream outlexf(fixlexfname);
    if (!outlexf) throw string("Problem opening lexicon file for writing.");

    string line;
    int linei = 0;
    while (getline(inlexf, line)) {
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
            if (dg.m_hmm_map.find(*pit) == dg.m_hmm_map.end())
                problem_phone = true;
        }

        if (problem_phone) {
            cerr << "Removed line " << linei << ", " << line << endl;
            continue;
        }
        else
            outlexf << line << endl;
    }
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: cleanlex PH LEXIN LEXOUT\n")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 3) config.print_help(stderr, 1);

    DecoderGraph dg;
    string phfname = config.arguments[0];
    dg.read_phone_model(phfname);

    string inlex = config.arguments[1];
    string outlex = config.arguments[2];
    fix_noway_lexicon(dg, inlex, outlex);

    exit(0);
}

