#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <cstdlib>
#include <iterator>

#include "conf.hh"
#include "DecoderGraph.hh"
#include "gutils.hh"

using namespace std;
using namespace gutils;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: wgraph [OPTION...] PH LEXICON WSEGS GRAPH\n")
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

        string segfname = config.arguments[2];
        cerr << "Reading segmentations: " << segfname << endl;
        vector<pair<string, vector<string> > > word_segs;
        dg.read_word_segmentations(segfname, word_segs);

        string graphfname = config.arguments[3];
        cerr << "Result graph file name: " << graphfname << endl;

        time_t rawtime;

        vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
        for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit) {
            vector<DecoderGraph::TriphoneNode> word_triphones;
            triphonize(dg, wit->second, word_triphones);
            dg.add_triphones(triphone_nodes, word_triphones);
        }

        vector<DecoderGraph::Node> nodes(2);
        dg.triphones_to_states(triphone_nodes, nodes);
        triphone_nodes.clear();
        dg.prune_unreachable_nodes(nodes);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Creating crossword network.." << endl;
        vector<DecoderGraph::Node> cw_nodes;
        map<string, int> fanout, fanin;
        dg.create_crossword_network(word_segs, cw_nodes, fanout, fanin);
        cerr << "Connecting crossword network.." << endl;
        dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
        dg.connect_end_to_start_node(nodes);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Pushing word ids right.." << endl;
        dg.push_word_ids_right(nodes);
        cerr << "Tying state chain prefixes.." << endl;
        dg.tie_state_prefixes(nodes);
        cerr << "number of nodes: " << dg.reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Pushing word ids left.." << endl;
        dg.push_word_ids_left(nodes);
        cerr << "Tying state chain suffixes.." << endl;
        dg.tie_state_suffixes(nodes);
        cerr << "number of nodes: " << dg.reachable_graph_nodes(nodes) << endl;

        dg.add_hmm_self_transitions(nodes);
        dg.write_graph(nodes, graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}