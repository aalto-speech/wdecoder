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
    config("usage: dgraph [OPTION...] PH DUR LEXICON WSEGS\n")
      ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);

    DecoderGraph dg;

    try {
        int assertions = 0;
        bool words_ok;

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

        cerr << "Expanding to phone graph.." << endl;
        vector<DecoderGraph::Node> nodes;
        dg.expand_subword_nodes(swnodes, nodes, false);
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
            //bool word_pairs_ok = assert_word_pairs(dg, nodes, false) );
        }

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Tying state chain prefixes.." << endl;
        dg.push_word_ids_right(nodes);
        dg.tie_state_prefixes(nodes, false);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;
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

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Tying state chain suffixes.." << endl;
        dg.push_word_ids_left(nodes);
        dg.tie_state_suffixes(nodes);
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;
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

        cerr << "Pushing subword ids.." << endl;
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

        dg.write_graph(nodes, segfname + ".graph");

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
