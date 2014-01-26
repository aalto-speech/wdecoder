#include <iostream>
#include <string>
#include <ctime>
#include <sstream>

#include "DecoderGraph.hh"
#include "conf.hh"

using namespace std;




void triphonize(string word,
                vector<string> &triphones)
{
    string tword = "_" + word + "_";
    triphones.clear();
    for (int i=1; i<tword.length()-1; i++) {
        stringstream tstring;
        tstring << tword[i-1] << "-" << tword[i] << "+" << tword[i+1];
        triphones.push_back(tstring.str());
    }
}


void triphonize(DecoderGraph &dg,
                string word,
                vector<string> &triphones)
{
    if (dg.m_word_segs.find(word) != dg.m_word_segs.end()) {
        string tripstring;
        for (auto swit = dg.m_word_segs[word].begin(); swit != dg.m_word_segs[word].end(); ++swit) {
            vector<string> &triphones = dg.m_lexicon[*swit];
            for (auto tit = triphones.begin(); tit != triphones.end(); ++tit)
                tripstring += (*tit)[2];
        }
        triphonize(tripstring, triphones);
    }
    else triphonize(word, triphones);
}


void triphonize_words(DecoderGraph &dg,
                      map<string, vector<string> > &triphonized_words) {
    for (auto wit = dg.m_word_segs.begin(); wit != dg.m_word_segs.end(); ++wit) {
        vector<string> triphones;
        triphonize(dg, wit->first, triphones);
        triphonized_words[wit->first] = triphones;
    }
}


bool
assert_path(DecoderGraph &dg,
            vector<DecoderGraph::Node> &nodes,
            deque<int> states,
            deque<string> subwords,
            int node_idx)
{
    DecoderGraph::Node &curr_node = nodes[node_idx];

    if (curr_node.hmm_state != -1) {
        if (states.size() == 0) return false;
        if (states.back() != curr_node.hmm_state) return false;
        else states.pop_back();
        if (states.size() == 0 && subwords.size() == 0) return true;
    }

    if (curr_node.word_id != -1) {
        if (subwords.size() == 0) return false;
        if (subwords.back() != dg.m_units[curr_node.word_id]) return false;
        else subwords.pop_back();
        if (states.size() == 0 && subwords.size() == 0) return true;
    }

    for (auto ait = curr_node.arcs.begin(); ait != curr_node.arcs.end(); ++ait) {
        bool retval = assert_path(dg, nodes, states, subwords, ait->target_node);
        if (retval) return true;
    }

    return false;
}


bool
assert_path(DecoderGraph &dg,
            vector<DecoderGraph::Node> &nodes,
            vector<string> &triphones,
            vector<string> &subwords,
            bool debug)
{
    deque<int> dstates;
    deque<string> dwords;

    for (auto tit = triphones.begin(); tit != triphones.end(); ++tit) {
        int hmm_index = dg.m_hmm_map[*tit];
        Hmm &hmm = dg.m_hmms[hmm_index];
        for (auto sit = hmm.states.begin(); sit != hmm.states.end(); ++sit)
            if (sit->model >= 0) dstates.push_front(sit->model);
    }
    for (auto wit = subwords.begin(); wit != subwords.end(); ++wit)
        dwords.push_front(*wit);

    if (debug) {
        cerr << "expecting hmm states: " << endl;
        for (auto it = dstates.rbegin(); it != dstates.rend(); ++it)
            cerr << " " << *it;
        cerr << endl;
        cerr << "expecting subwords: " << endl;
        for (auto it = subwords.begin(); it != subwords.end(); ++it)
            cerr << " " << *it;
        cerr << endl;
    }

    return assert_path(dg, nodes, dstates, dwords, DecoderGraph::START_NODE);
}


bool
assert_words(DecoderGraph &dg,
             vector<DecoderGraph::Node> &nodes,
             map<string, vector<string> > &triphonized_words,
             bool debug)
{
    for (auto sit=dg.m_word_segs.begin(); sit!=dg.m_word_segs.end(); ++sit) {
        bool result = assert_path(dg, nodes, triphonized_words[sit->first], sit->second, false);
        if (!result) {
            cerr << "error, word: " << sit->first << " not found" << endl;
            return false;
        }
    }
    return true;
}



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
            triphonize_words(dg, triphonized_words);
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
        vector<DecoderGraph::Node> nodes(2);
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
        nodes[DecoderGraph::END_NODE].arcs.resize(nodes[1].arcs.size()+1);
        nodes[DecoderGraph::END_NODE].arcs.back().target_node = DecoderGraph::START_NODE;
        cerr << "number of hmm state nodes: " << dg.reachable_graph_nodes(nodes) << endl;
        if (assertions) {
            words_ok = assert_words(dg, nodes, triphonized_words, false);
            cerr << "assert_words: " << words_ok << endl;
            //bool word_pairs_ok = assert_word_pairs(dg, nodes, false) );
        }

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;
        cerr << "Tying state chain prefixes.." << endl;
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
        dg.tie_state_suffixes(nodes, DecoderGraph::END_NODE);
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
        dg.push_word_ids_left(nodes);
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

        //dg.print_graph(nodes);

        time ( &rawtime );
        cerr << "time: " << ctime (&rawtime) << endl;

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
