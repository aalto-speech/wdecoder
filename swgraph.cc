#include "conf.hh"
#include "SubwordGraphBuilder.hh"
#include "gutils.hh"

using namespace std;
using namespace gutils;
using namespace subwordgraphbuilder;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: swgraph [OPTION...] PH LEXICON GRAPH\n")
    ('n', "no-start-end-wb", "", "", "No word boundary symbol in the beginning and end of sentences")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 3) config.print_help(stderr, 1);
    bool no_start_end_wb = config["no-start-end-wb"].specified;

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

        set<string> subwords;
        vector<DecoderGraph::Node> nodes(2);
        for (auto swit = dg.m_lexicon.begin(); swit != dg.m_lexicon.end(); ++swit) {
            subwords.insert(swit->first);
            if (swit->second.size() < 2) continue;
            vector<TriphoneNode> sw_triphones;
            triphonize_subword(dg, swit->first, sw_triphones);
            vector<DecoderGraph::Node> sw_nodes;
            triphones_to_state_chain(dg, sw_triphones, sw_nodes);
            add_nodes_to_tree(dg, nodes, sw_nodes);
        }
        lookahead_to_arcs(nodes);

        prune_unreachable_nodes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        set<node_idx_t> third_nodes;
        set_reverse_arcs(nodes);
        find_nodes_in_depth_reverse(nodes, third_nodes, dg.m_states_per_phone+1);
        clear_reverse_arcs(nodes);
        for (auto nii=third_nodes.begin(); nii != third_nodes.end(); ++nii)
            nodes[*nii].flags |= NODE_LM_RIGHT_LIMIT;

        third_nodes.clear();
        find_nodes_in_depth(nodes, third_nodes, dg.m_states_per_phone+1);
        for (auto nii=third_nodes.begin(); nii !=third_nodes.end(); ++nii)
            nodes[*nii].flags |= NODE_LM_LEFT_LIMIT;

        push_word_ids_right(nodes);
        cerr << "Tying state prefixes.." << endl;
        tie_state_prefixes(nodes);
        push_word_ids_left(nodes);
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        vector<DecoderGraph::Node> cw_nodes;
        map<string, int> fanout, fanin;
        cerr << "Creating crossword network.." << endl;
        create_crossword_network(dg, subwords, cw_nodes, fanout, fanin);
        cerr << "crossword network size: " << cw_nodes.size() << endl;
        minimize_crossword_network(cw_nodes, fanout, fanin);
        cerr << "tied crossword network size: " << cw_nodes.size() << endl;

        cerr << "Connecting crossword network.." << endl;
        connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin, false);
        connect_end_to_start_node(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        connect_one_phone_subwords_from_start_to_cw(dg, subwords, nodes, fanout);
        connect_one_phone_subwords_from_cw_to_end(dg, subwords, nodes, fanin);
        prune_unreachable_nodes(nodes);

        cerr << "Removing cw dummies.." << endl;
        remove_cw_dummies(nodes);
        tie_state_suffixes(nodes);
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        if (no_start_end_wb)
            add_long_silence_no_start_end_wb(dg, nodes);
        else {
            add_long_silence(dg, nodes);
            nodes[END_NODE].word_id = dg.m_subword_map["<w>"];
        }

        add_hmm_self_transitions(nodes);

        write_graph(nodes, graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
