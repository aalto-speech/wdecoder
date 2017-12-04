#include "conf.hh"
#include "LRWBSubwordGraph.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: lrwbswgraph [OPTION...] PH LEXICON GRAPH\n")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 3) config.print_help(stderr, 1);

    LRWBSubwordGraph swg;

    try {
        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        swg.read_phone_model(phfname);

        set<string> prefix_subwords, stem_subwords, suffix_subwords, word_subwords;
        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        swg.read_lexicon(lexfname,
                         prefix_subwords,
                         stem_subwords,
                         suffix_subwords,
                         word_subwords);

        string graphfname = config.arguments[2];
        cerr << "Result graph file name: " << graphfname << endl;

        swg.create_graph(
                prefix_subwords,
                stem_subwords,
                suffix_subwords,
                word_subwords,
                true);
        swg.add_long_silence();
        swg.add_hmm_self_transitions();
        swg.write_graph(graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
