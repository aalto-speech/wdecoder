#include "conf.hh"
#include "GraphBuilder.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: swwgraph [OPTION...] PH LEXICON WSEGS GRAPH\n")
    ('b', "word-boundary", "", "", "Use word boundary symbol (<w>)")
    ('n', "no-push", "", "", "Don't move subword identifiers in the graph")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);
    bool wb_symbol = config["word-boundary"].specified;
    bool no_push = config["no-push"].specified;

    SWWGraph swwg;

    try {
        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        swwg.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        swwg.read_noway_lexicon(lexfname);

        string segfname = config.arguments[2];
        cerr << "Reading segmentations: " << segfname << endl;
        map<string, vector<string> > word_segs;
        swwg.read_word_segmentations(segfname, word_segs);

        string graphfname = config.arguments[3];
        cerr << "Result graph file name: " << graphfname << endl;

        swwg.create_graph(word_segs, wb_symbol, true, true);

        swwg.tie_graph(no_push, true);

        if (wb_symbol) swwg.m_nodes[END_NODE].word_id = swwg.m_subword_map["<w>"];
        swwg.add_long_silence();
        swwg.add_hmm_self_transitions();

        swwg.write_graph(graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
