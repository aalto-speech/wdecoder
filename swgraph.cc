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

        vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
        set<string> subwords;
        for (auto swit = dg.m_lexicon.begin(); swit != dg.m_lexicon.end(); ++swit) {
            subwords.insert(swit->first);
            if (swit->second.size() < 2) continue;
            vector<DecoderGraph::TriphoneNode> sw_triphones;
            triphonize_subword(dg, swit->first, sw_triphones);
            add_triphones(triphone_nodes, sw_triphones);
        }

        vector<DecoderGraph::Node> nodes(2);
        triphones_to_states(dg, triphone_nodes, nodes);
        triphone_nodes.clear();
        prune_unreachable_nodes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        vector<DecoderGraph::Node> cw_nodes;
        map<string, int> fanout, fanin;
        cerr << "Creating crossword network.." << endl;
        create_crossword_network(dg, subwords, cw_nodes, fanout, fanin);

        cerr << "Connecting crossword network.." << endl;
        connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin, false);
        connect_end_to_start_node(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        connect_one_phone_subwords_from_start_to_cw(dg, subwords, nodes, fanout);
        connect_one_phone_subwords_from_cw_to_end(dg, subwords, nodes, fanin);
        prune_unreachable_nodes(nodes);

        cerr << endl;
        cerr << "Tying state prefixes.." << endl;
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Removing cw dummies.." << endl;
        remove_cw_dummies(nodes);

        cerr << "Tying prefixes.." << endl;
        tie_state_prefixes(nodes);
        tie_word_id_prefixes(nodes);
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying suffixes.." << endl;
        tie_state_suffixes(nodes);
        tie_word_id_suffixes(nodes);
        tie_state_suffixes(nodes);
        tie_word_id_suffixes(nodes);
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        add_long_silence(dg, nodes);
        add_hmm_self_transitions(nodes);
        nodes[END_NODE].word_id = dg.m_subword_map["<w>"];

        write_graph(nodes, graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
