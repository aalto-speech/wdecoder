#include <iostream>
#include <string>
#include <ctime>
#include <climits>

#include "Decoder.hh"
#include "LM.hh"
#include "conf.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: dgraph [OPTION...] PH DUR LEXICON LM GRAPH\n")
      ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 5) config.print_help(stderr, 1);

    try {

        Decoder d;

        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        d.read_phone_model(phfname);

        string durfname = config.arguments[1];
        cerr << "Reading duration models: " << durfname << endl;
        d.read_duration_model(durfname);

        string lexfname = config.arguments[2];
        cerr << "Reading lexicon: " << lexfname << endl;
        d.read_noway_lexicon(lexfname);

        string lmfname = config.arguments[3];
        cerr << "Reading language model: " << lmfname << endl;
        d.read_lm(lmfname);

        string graphfname = config.arguments[4];
        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);

        cerr << "node count: " << d.m_nodes.size() << endl;


        for (int i=0; i<d.m_subwords.size(); i++) {
            string blaa(d.m_subwords[i]);
            int sym = d.m_lm.symbol_map().index(blaa);
            cerr << i << " " << sym << endl;
        }

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
