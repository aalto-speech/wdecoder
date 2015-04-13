#include <sstream>

#include "conf.hh"
#include "gutils.hh"
#include "WordGraphBuilder.hh"
#include "GraphBuilder.hh"

using namespace std;
using namespace gutils;


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

        string wordfname = config.arguments[2];
        cerr << "Reading word list: " << wordfname << endl;
        set<string> words;
        read_words(dg, wordfname, words);

        string graphfname = config.arguments[3];
        cerr << "Result graph file name: " << graphfname << endl;

        vector<DecoderGraph::Node> nodes(2);
        wordgraphbuilder::create_graph(dg, words, nodes, true);
        wordgraphbuilder::tie_graph(nodes, true);

        add_long_silence(dg, nodes);
        add_hmm_self_transitions(nodes);

        write_graph(nodes, graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
