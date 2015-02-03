
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

        set<node_idx_t> third_nodes;
        set_reverse_arcs(nodes);
        find_nodes_in_depth_reverse(nodes, third_nodes, 4);
        clear_reverse_arcs(nodes);
        for (auto nii=third_nodes.begin(); nii != third_nodes.end(); ++nii)
            nodes[*nii].flags |= NODE_LM_RIGHT_LIMIT;

        third_nodes.clear();
        find_nodes_in_depth(nodes, third_nodes, 4);
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
        tie_state_prefixes_cw(cw_nodes, fanout, fanin);
        tie_word_id_prefixes_cw(cw_nodes, fanout, fanin);
        tie_state_prefixes_cw(cw_nodes, fanout, fanin);
        tie_word_id_prefixes_cw(cw_nodes, fanout, fanin);
        tie_state_suffixes_cw(cw_nodes, fanout, fanin);
        tie_word_id_suffixes_cw(cw_nodes, fanout, fanin);
        tie_state_suffixes_cw(cw_nodes, fanout, fanin);
        tie_word_id_suffixes_cw(cw_nodes, fanout, fanin);
        tie_state_suffixes_cw(cw_nodes, fanout, fanin);
        tie_word_id_suffixes_cw(cw_nodes, fanout, fanin);
        tie_state_suffixes_cw(cw_nodes, fanout, fanin);
        tie_word_id_suffixes_cw(cw_nodes, fanout, fanin);
        tie_state_prefixes_cw(cw_nodes, fanout, fanin);
        tie_word_id_prefixes_cw(cw_nodes, fanout, fanin);
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

        add_long_silence(dg, nodes, true);
        add_hmm_self_transitions(nodes);

        write_graph(nodes, graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
