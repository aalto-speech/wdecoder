#include "conf.hh"
#include "gutils.hh"
#include "GraphBuilder.hh"

using namespace std;
using namespace gutils;
using namespace graphbuilder;


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

        vector<DecoderGraph::Node> nodes(2);
        graphbuilder::create_graph(dg, nodes, word_segs, wb_symbol, true);

        cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying word id suffixes.." << endl;
        tie_word_id_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying word id suffixes.." << endl;
        tie_word_id_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying word id suffixes.." << endl;
        tie_word_id_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying word id suffixes.." << endl;
        tie_word_id_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying word id suffixes.." << endl;
        tie_word_id_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Removing cw dummies.." << endl;
        remove_cw_dummies(nodes);
        cerr << "Tying state prefixes.." << endl;
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        if (!no_push) {
            cerr << "Pushing subword ids right.." << endl;
            push_word_ids_right(nodes);
            cerr << "Tying state prefixes.." << endl;
            tie_state_prefixes(nodes);
            cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

            cerr << "Pushing subword ids left.." << endl;
            push_word_ids_left(nodes);
            cerr << "Tying state suffixes.." << endl;
            tie_state_suffixes(nodes);
            cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        }

        if (wb_symbol) nodes[END_NODE].word_id = dg.m_subword_map["<w>"];
        add_long_silence(dg, nodes);
        add_hmm_self_transitions(nodes);

        write_graph(nodes, graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
