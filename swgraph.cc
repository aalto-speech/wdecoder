#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <cstdlib>
#include <iterator>

#include "conf.hh"
#include "DecoderGraph.hh"
#include "SubwordGraphBuilder.hh"
#include "gutils.hh"

using namespace std;
using namespace gutils;
using namespace subwordgraphbuilder;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: dgraph [OPTION...] PH LEXICON GRAPH\n")
      ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 3) config.print_help(stderr, 1);

    DecoderGraph swg;

    try {
        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        swg.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        swg.read_noway_lexicon(lexfname);

        string graphfname = config.arguments[2];
        cerr << "Result graph file name: " << graphfname << endl;

        vector<TriphoneNode> triphone_nodes(2);
        for (auto swit = swg.m_lexicon.begin(); swit != swg.m_lexicon.end(); ++swit) {
            if (swit->second.size() < 2) continue;
            vector<TriphoneNode> sw_triphones;
            triphonize_subword(swg, swit->first, sw_triphones);
            swg.add_triphones(triphone_nodes, sw_triphones);
        }

        vector<DecoderGraph::Node> nodes(2);
        triphones_to_states(triphone_nodes, nodes);
        triphone_nodes.clear();
        prune_unreachable_nodes(nodes);
        cerr << "number of nodes: " << swg.reachable_graph_nodes(nodes) << endl;

        vector<DecoderGraph::Node> cw_nodes;
        map<string, int> fanout, fanin;
        cerr << "Creating crossword network.." << endl;
        create_crossword_network(dg, cw_nodes, fanout, fanin);

        cerr << "Connecting crossword network.." << endl;
        connect_crossword_network(nodes, cw_nodes, fanout, fanin, true);
        connect_end_to_start_node(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        connect_one_phone_subwords_from_start_to_cw(swg, nodes, fanout);
        connect_one_phone_subwords_from_cw_to_end(swg, nodes, fanin);

        cerr << "Tying state chain suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << swg.reachable_graph_nodes(nodes) << endl;

        cerr << "Tying state chain prefixes.." << endl;
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << swg.reachable_graph_nodes(nodes) << endl;

        add_hmm_self_transitions(nodes);
        write_graph(nodes, graphfname);
        //swg.print_dot_digraph(nodes);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
