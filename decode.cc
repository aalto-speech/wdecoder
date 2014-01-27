#include <iostream>
#include <string>
#include <ctime>

#include "Decoder.hh"
#include "conf.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: dgraph [OPTION...] PH DUR LEXICON GRAPH\n")
      ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);

    try {
        cout << "decoder" << endl;
    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
