#include "conf.hh"
#include "NoWBSubwordGraph.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: nowbswgraph [OPTION...] PH LEXICON GRAPH\n")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 3) config.print_help(stderr, 1);

    NoWBSubwordGraph swg;

    try {
        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        swg.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        swg.read_noway_lexicon(lexfname);

        string graphfname = config.arguments[2];
        cerr << "Result graph file name: " << graphfname << endl;

        set<string> subwords;
        for (auto swit = swg.m_lexicon.begin(); swit != swg.m_lexicon.end(); ++swit)
            subwords.insert(swit->first);

        swg.create_graph(subwords, true);
        swg.add_long_silence();
        swg.add_hmm_self_transitions();
        swg.write_graph(graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
