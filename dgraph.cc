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
    config("usage: dgraph [OPTION...] PH LEXICON WSEGS GRAPH\n")
      ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);

    DecoderGraph dg;

    try {
        int assertions = 1;
        bool words_ok;

        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        dg.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        dg.read_noway_lexicon(lexfname);

        string segfname = config.arguments[2];
        cerr << "Reading segmentations: " << segfname << endl;
        dg.read_word_segmentations(segfname);

        string graphfname = config.arguments[3];
        cerr << "Result graph file name: " << graphfname << endl;

        map<string, vector<string> > triphonized_words;
        if (assertions) {
            cerr << "Triphonizing words" << endl;
            triphonize_all_words(dg, triphonized_words);
        }

        time_t rawtime;
        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Creating subword graph.." << endl;
        vector<DecoderGraph::SubwordNode> swnodes;
        dg.create_word_graph(swnodes);
        cerr << "node count: " << dg.reachable_word_graph_nodes(swnodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Tying subword suffixes.." << endl;
        dg.tie_subword_suffixes(swnodes);
        cerr << "node count: " << dg.reachable_word_graph_nodes(swnodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Expanding to phone graph.." << endl;
        vector<DecoderGraph::Node> nodes;
        dg.expand_subword_nodes(swnodes, nodes);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;
        if (assertions) {
            words_ok = assert_words(dg, nodes, triphonized_words, false);
            cerr << "assert_words: " << words_ok << endl;
        }

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Creating crossword network.." << endl;
        vector<DecoderGraph::Node> cw_nodes;
        map<string, int> fanout, fanin;
        dg.create_crossword_network(cw_nodes, fanout, fanin);
        cerr << "Connecting crossword network.." << endl;
        dg.connect_crossword_network(nodes, cw_nodes, fanout, fanin);
        dg.connect_end_to_start_node(nodes);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;
        if (assertions) {
            words_ok = assert_words(dg, nodes, triphonized_words, false);
            cerr << "assert_words: " << words_ok << endl;
        }

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Tying state chain prefixes.." << endl;
        dg.push_word_ids_right(nodes);
        dg.tie_state_prefixes(nodes, false);
        dg.prune_unreachable_nodes(nodes);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;
        if (assertions) {
            words_ok = assert_words(dg, nodes, triphonized_words, false);
            cerr << "assert_words: " << words_ok << endl;
        }

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Tying state chain suffixes.." << endl;
        dg.push_word_ids_left(nodes);
        dg.tie_state_suffixes(nodes);
        dg.prune_unreachable_nodes(nodes);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;
        if (assertions) {
            words_ok = assert_words(dg, nodes, triphonized_words, false);
            cerr << "assert_words: " << words_ok << endl;
        }

        cerr << "Pushing subword ids right.." << endl;
        dg.push_word_ids_right(nodes);
        if (assertions) {
            words_ok = assert_words(dg, nodes, triphonized_words, false);
            cerr << "assert_words: " << words_ok << endl;
        }

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Pruning unreachable nodes.." << endl;
        dg.prune_unreachable_nodes(nodes);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;
        if (assertions) {
            words_ok = assert_words(dg, nodes, triphonized_words, false);
            cerr << "assert_words: " << words_ok << endl;
        }

        if (assertions) {
            bool pairs_ok = assert_word_pairs(dg, nodes, 10000, false);
            cerr << "assert word pairs: " << pairs_ok << endl;
        }

        dg.write_graph(nodes, graphfname);

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
