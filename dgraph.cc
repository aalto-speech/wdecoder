#include <iostream>
#include <string>
#include <ctime>

#include "DecoderGraph.hh"
#include "conf.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: dgraph [OPTION...] PH DUR LEXICON WSEGS\n")
      ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);

    DecoderGraph dg;

    try {
        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        dg.read_phone_model(phfname);

        string durfname = config.arguments[1];
        cerr << "Reading duration models: " << durfname << endl;
        dg.read_duration_model(durfname);

        string lexfname = config.arguments[2];
        cerr << "Reading lexicon: " << lexfname << endl;
        dg.read_noway_lexicon(lexfname);

        string segfname = config.arguments[3];
        cerr << "Reading segmentations: " << segfname << endl;
        dg.read_word_segmentations(segfname);

        time_t rawtime;
        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

        cerr << "Creating subword graph.." << endl;
        vector<DecoderGraph::SubwordNode> swnodes;
        dg.create_word_graph(swnodes);
        cerr << "node count: " << dg.reachable_word_graph_nodes(swnodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

        cerr << "Tying suffixes.." << endl;
        dg.tie_subword_suffixes(swnodes);
        cerr << "node count: " << dg.reachable_word_graph_nodes(swnodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

        cerr << "Expanding to phone graph.." << endl;
        vector<DecoderGraph::Node> nodes(2);
        dg.expand_subword_nodes(swnodes, nodes, false);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

        cerr << "Tying state chain prefixes.." << endl;
        dg.tie_state_prefixes(nodes, 0, DecoderGraph::START_NODE);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

        cerr << "Pruning unreachable nodes.." << endl;
        dg.prune_unreachable_nodes(nodes);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

        cerr << "Pushing subword ids.." << endl;
        //dg.push_word_ids_left(nodes);

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

        cerr << "Pruning unreachable nodes.." << endl;
        dg.prune_unreachable_nodes(nodes);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;

        dg.print_graph(nodes);

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
