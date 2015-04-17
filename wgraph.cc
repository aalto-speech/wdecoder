#include <sstream>

#include "conf.hh"
#include "WordGraph.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: wgraph [OPTION...] PH LEXICON WORDS GRAPH\n")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);

    DecoderGraph dg;

    try {
        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        dg.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        dg.read_noway_lexicon(lexfname);

        WordGraph wg;
        string wordfname = config.arguments[2];
        cerr << "Reading word list: " << wordfname << endl;
        set<string> words;
        wg.read_words(wordfname, words);

        string graphfname = config.arguments[3];
        cerr << "Result graph file name: " << graphfname << endl;

        wg.create_graph(words, true);
        wg.tie_graph(true);

        wg.add_long_silence();
        wg.add_hmm_self_transitions();

        wg.write_graph(graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
