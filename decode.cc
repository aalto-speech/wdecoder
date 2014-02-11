#include <iostream>
#include <string>
#include <ctime>
#include <climits>

#include "Decoder.hh"
#include "LM.hh"
#include "conf.hh"

using namespace std;


void print_graph(Decoder &d, string fname) {
    ofstream outf(fname);
    d.print_dot_digraph(d.m_nodes, outf);
    outf.close();
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: dgraph [OPTION...] PH LEXICON LM CFGFILE GRAPH LNALIST\n")
      ('h', "help", "", "", "display help")
      ('d', "duration-model=STRING", "arg", "", "Duration model");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 6) config.print_help(stderr, 1);

    try {

        Decoder d;

        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        d.read_phone_model(phfname);

        if (config["duration-model"].specified) {
            string durfname = config["duration-model"].get_str();
            cerr << "Reading duration model: " << durfname << endl;
            d.read_duration_model(durfname);
        }

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        d.read_noway_lexicon(lexfname);

        string lmfname = config.arguments[2];
        cerr << "Reading language model: " << lmfname << endl;
        d.read_lm(lmfname);

        string cfgfname = config.arguments[3];
        cerr << "Reading configuration: " << cfgfname << endl;
        d.read_config(cfgfname);
        d.print_config(cerr);

        string graphfname = config.arguments[4];
        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);
        cerr << "node count: " << d.m_nodes.size() << endl;

        string lnalistfname = config.arguments[5];
        ifstream lnalistf(lnalistfname);
        string line;

        cerr << endl;
        while (getline(lnalistf, line)) {
            if (!line.length()) continue;
            cerr << "recognizing: " << line << endl;
            d.recognize_lna_file(line);
        }
        lnalistf.close();

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
