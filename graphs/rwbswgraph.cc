#include "conf.hh"
#include "RWBSubwordGraph.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: rwbswgraph [OPTION...] PH LEXICON GRAPH\n")
    ('o', "omit-sentence-end-symbol", "", "", "No sentence end symbol in the silence loop")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 3) config.print_help(stderr, 1);
    bool sentence_end_symbol = !(config["omit-sentence-end-symbol"].specified);

    RWBSubwordGraph swg;

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
            if (swit->first.back() == '#' || swit->first.back() == '_') suffix_subwords.insert(swit->first);
            else prefix_subwords.insert(swit->first);
        }
        cerr << "Number of prefix/stem subwords: " << prefix_subwords.size() << endl;
        cerr << "Number of suffix subwords: " << suffix_subwords.size() << endl;

        swg.create_graph(prefix_subwords, suffix_subwords, true);
        swg.add_silence_loop(sentence_end_symbol);
        swg.add_hmm_self_transitions();
        swg.write_graph(graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
