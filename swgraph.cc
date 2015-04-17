#include "conf.hh"
#include "SubwordGraph.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: swgraph [OPTION...] PH LEXICON GRAPH\n")
    ('n', "no-start-end-wb", "", "", "No word boundary symbol in the beginning and end of sentences")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 3) config.print_help(stderr, 1);
    bool no_start_end_wb = config["no-start-end-wb"].specified;

    DecoderGraph dg;

    try {
        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        dg.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        dg.read_noway_lexicon(lexfname);

        string graphfname = config.arguments[2];
        cerr << "Result graph file name: " << graphfname << endl;

        set<string> subwords;
        for (auto swit = dg.m_lexicon.begin(); swit != dg.m_lexicon.end(); ++swit)
            subwords.insert(swit->first);

        SubwordGraph swg;
        swg.create_graph(subwords, true);

        if (no_start_end_wb)
            swg.add_long_silence_no_start_end_wb();
        else {
            swg.add_long_silence();
            swg.m_nodes[END_NODE].word_id = dg.m_subword_map["<w>"];
        }

        swg.add_hmm_self_transitions();

        swg.write_graph(graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
