#include <iostream>
#include <string>

#include "DecoderGraph.hh"
#include "conf.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: dgraph [OPTION...] PH DUR LEXICON\n")
      ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 3) config.print_help(stderr, 1);

    DecoderGraph dg;

    string phfname = config.arguments[0];
    cerr << "Reading hmms: " << phfname << endl;
    dg.read_phone_model(phfname);

    string durfname = config.arguments[1];
    cerr << "Reading duration models: " << durfname << endl;
    dg.read_duration_model(durfname);

    string lexfname = config.arguments[2];
    cerr << "Reading lexicon: " << lexfname << endl;
    dg.read_noway_lexicon(lexfname);

    exit(1);
}
