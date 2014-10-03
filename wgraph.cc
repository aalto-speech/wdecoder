#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <cstdlib>
#include <iterator>

#include "conf.hh"
#include "DecoderGraph.hh"
#include "gutils.hh"
#include "WordGraphBuilder.hh"
#include "GraphBuilder2.hh"

using namespace std;
using namespace gutils;


void
read_words(DecoderGraph &dg,
           string wordfname,
           set<string> &words)
{
    ifstream wordf(wordfname);
    if (!wordf) throw string("Problem opening word list.");

    string line;
    int linei = 1;
    while (getline(wordf, line)) {
        string word;
        stringstream ss(line);
        ss >> word;
        if (dg.m_subword_map.find(word) == dg.m_subword_map.end())
            throw "Word " + word + " not found in lexicon";
        words.insert(word);
        linei++;
    }
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: wgraph [OPTION...] PH LEXICON WORDS GRAPH\n")
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

        string wordfname = config.arguments[2];
        cerr << "Reading word list: " << wordfname << endl;
        set<string> words;
        read_words(dg, wordfname, words);

        string graphfname = config.arguments[3];
        cerr << "Result graph file name: " << graphfname << endl;

        vector<DecoderGraph::TriphoneNode> triphone_nodes(2);
        for (auto wit = words.begin(); wit != words.end(); ++wit) {
            vector<DecoderGraph::TriphoneNode> word_triphones;
            triphonize_subword(dg, *wit, word_triphones);
            if (word_triphones.size() == 2)
                cerr << "skipping one phone word: " << *wit << endl;
            add_triphones(triphone_nodes, word_triphones);
        }

        vector<DecoderGraph::Node> nodes(2);
        triphones_to_states(dg, triphone_nodes, nodes);
        triphone_nodes.clear();
        prune_unreachable_nodes(nodes);
        cerr << "number of hmm state nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << "Creating crossword network.." << endl;
        vector<DecoderGraph::Node> cw_nodes;
        map<string, int> fanout, fanin;
        wordgraphbuilder::create_crossword_network(dg, words, cw_nodes, fanout, fanin);

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
        graphbuilder2::connect_crossword_network(dg, nodes, cw_nodes, fanout, fanin);
        connect_end_to_start_node(nodes);
        cerr << "number of hmm state nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Pushing word ids right.." << endl;
        push_word_ids_right(nodes);
        cerr << "Tying state prefixes.." << endl;
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        cerr << "Pushing word ids left.." << endl;
        push_word_ids_left(nodes);
        cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << endl;
        //cerr << "Removing cw dummies.." << endl;
        //remove_cw_dummies(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;
        cerr << "Tying state suffixes.." << endl;
        tie_state_suffixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        cerr << "Tying state prefixes.." << endl;
        tie_state_prefixes(nodes);
        cerr << "number of nodes: " << reachable_graph_nodes(nodes) << endl;

        add_long_silence(dg, nodes);
        add_hmm_self_transitions(nodes);

        write_graph(nodes, graphfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
