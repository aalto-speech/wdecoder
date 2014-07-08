#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <cstdlib>
#include <iterator>

#include "conf.hh"
#include "DecoderGraph.hh"
#include "gutils.hh"
#include "GraphBuilder1.hh"

using namespace std;
using namespace gutils;
using namespace graphbuilder1;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: dgraph [OPTION...] PH LEXICON WSEGS GRAPH\n")
    ('b', "word-boundary", "", "", "Use word boundary symbol (<w>)")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);
    bool wb_symbol = config["word-boundary"].specified;

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
        map<string, vector<string> > word_segs;
        read_word_segmentations(dg, segfname, word_segs);

        string graphfname = config.arguments[3];
        cerr << "Result graph file name: " << graphfname << endl;

        time_t rawtime;
        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Creating subword graph.." << endl;
        vector<SubwordNode> swnodes;
        create_word_graph(dg, swnodes, word_segs);
        cerr << "node count: " << reachable_word_graph_nodes(swnodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Tying subword suffixes.." << endl;
        tie_subword_suffixes(swnodes);
        cerr << "node count: " << reachable_word_graph_nodes(swnodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Expanding to phone graph.." << endl;
        vector<DecoderGraph::Node> nodes;
        expand_subword_nodes(dg, swnodes, nodes);
        swnodes.clear();
        cerr << "number of hmm state nodes: " << reachable_graph_nodes(nodes) << endl;

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Creating crossword network.." << endl;
        vector<DecoderGraph::Node> cw_nodes;
        map<string, int> fanout, fanin;
        create_crossword_network(dg, word_segs, cw_nodes, fanout, fanin, wb_symbol);
        cerr << "Connecting crossword network.." << endl;
        connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin, true);
        connect_end_to_start_node(nodes);
        cerr << "number of hmm state nodes: " << reachable_graph_nodes(nodes) << endl;
        cw_nodes.clear();

        cerr << "Setting word boundary symbols.." << endl;
        if (wb_symbol)
        {
            for (unsigned int i=0; i<nodes.size(); i++)
                if (nodes[i].flags & NODE_FAN_OUT_DUMMY) {
                    nodes[i].flags = 0;
                    nodes[i].word_id =  dg.m_subword_map["<w>"];
                }
        }

        /*
        cerr << endl;
        cerr << "Pushing subword ids left.." << endl;
        push_word_ids_left(nodes);
        cerr << "Tying state chain prefixes.." << endl;
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying word id prefixes.." << endl;
        tie_word_id_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Pushing subword ids left.." << endl;
        push_word_ids_left(nodes);
        cerr << "Tying state chain prefixes.." << endl;
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying word id prefixes.." << endl;
        tie_word_id_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Pushing subword ids right.." << endl;
        push_word_ids_right(nodes);
        cerr << "Tying state chain prefixes.." << endl;
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        */

        /*
        cerr << endl;
        cerr << "Pushing subword ids right.." << endl;
        push_word_ids_right(nodes);
        cerr << "Tying state chain suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying word id suffixes.." << endl;
        tie_word_id_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Pushing subword ids right.." << endl;
        push_word_ids_right(nodes);
        cerr << "Tying state chain suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying word id suffixes.." << endl;
        tie_word_id_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Pushing subword ids left.." << endl;
        push_word_ids_left(nodes);
        cerr << "Tying state chain suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        */

        cerr << "Pushing subword ids right.." << endl;
        push_word_ids_right(nodes);

        cerr << "Tying word id suffixes.." << endl;
        tie_word_id_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << "Tying state chain suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << "Tying word id suffixes.." << endl;
        tie_word_id_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << "Tying state chain suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << "Tying state chain prefixes.." << endl;
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << "Pushing subword ids left.." << endl;
        push_word_ids_left(nodes);

        cerr << "Tying word id prefixes.." << endl;
        tie_word_id_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << "Tying state chain prefixes.." << endl;
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        if (wb_symbol) nodes[END_NODE].word_id = dg.m_subword_map["<w>"];
        add_long_silence(dg, nodes);
        add_hmm_self_transitions(nodes);

        write_graph(nodes, graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
