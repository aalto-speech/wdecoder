#include <iostream>
#include <string>

#include "DecoderGraph.hh"
#include "conf.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: dgraph [OPTION...] LEXICON\n")
      ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 1) config.print_help(stderr, 1);

    DecoderGraph dg;

    string lexfname = config.arguments[0];
    cerr << "Reading lexicon: " << lexfname << endl;
    dg.read_noway_lexicon(lexfname);

    exit(1);
}

