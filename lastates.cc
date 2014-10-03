#include <iostream>
#include <string>
#include <ctime>
#include <climits>

#include "Decoder.hh"
#include "Lookahead.hh"
#include "conf.hh"
#include "io.hh"

using namespace std;


void print_graph(Decoder &d, string fname) {
    ofstream outf(fname);
    d.print_dot_digraph(d.m_nodes, outf);
    outf.close();
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: lastates [OPTION...] PH LEXICON GRAPH LALM LASTATES\n")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 5) config.print_help(stderr, 1);

    try {
        Decoder d;

        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        d.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        d.read_noway_lexicon(lexfname);

        string graphfname = config.arguments[2];
        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);
        cerr << "node count: " << d.m_nodes.size() << endl;

        string lalmfname = config.arguments[3];
        cerr << "Reading lookahead model: " << lalmfname << endl;
        LargeBigramLookahead lbla(d, lalmfname);

        string lasfname = config.arguments[4];
        cerr << "Writing lookahead states: " << lasfname << endl;
        lbla.write(lasfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
