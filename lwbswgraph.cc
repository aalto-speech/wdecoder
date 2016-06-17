#include "conf.hh"
#include "LWBSubwordGraph.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: lwbswgraph [OPTION...] PH LEXICON GRAPH\n")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 3) config.print_help(stderr, 1);

    LWBSubwordGraph swg;

    try {
        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        swg.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        swg.read_noway_lexicon(lexfname);

        string graphfname = config.arguments[2];
        cerr << "Result graph file name: " << graphfname << endl;

        set<string> prefix_subwords, suffix_subwords;
        for (auto swit = swg.m_lexicon.begin(); swit != swg.m_lexicon.end(); ++swit) {
            if (swit->first.find("<") != string::npos) continue;
            if (swit->first[0] == '#' || swit->first[0] == '_') prefix_subwords.insert(swit->first);
            else suffix_subwords.insert(swit->first);
        }
        cerr << "Number of prefix subwords: " << prefix_subwords.size() << endl;
        cerr << "Number of stem/suffix subwords: " << suffix_subwords.size() << endl;

        swg.create_graph(prefix_subwords, suffix_subwords, true);
        swg.add_long_silence();
        swg.add_hmm_self_transitions();
        swg.write_graph(graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
