#include <sstream>

#include "conf.hh"
#include "WordGraph.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: wgraph [OPTION...] PH LEXICON WORDS GRAPH\n")
    ('l', "word-labels", "", "", "Write word labels instead of indices")
    ('o', "omit-sentence-end-symbol", "", "", "No sentence end symbol in the silence loop")
    ('n', "no-tying", "", "", "Only cross-word network tied, main graph kept as a lexical prefix tree")
    ('r', "remove-cw-markers", "", "", "Remove the cross-word markers for the most compact tying")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);
    bool word_labels = config["word-labels"].specified;
    bool sentence_end_symbol = !(config["omit-sentence-end-symbol"].specified);
    bool no_tying = config["no-tying"].specified;
    bool remove_cw_markers = config["remove-cw-markers"].specified;

    WordGraph wg;

    try {
        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        wg.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        wg.read_noway_lexicon(lexfname);

        string wordfname = config.arguments[2];
        cerr << "Reading word list: " << wordfname << endl;
        set<string> words;
        wg.read_words(wordfname, words);

        string graphfname = config.arguments[3];
        cerr << "Result graph file name: " << graphfname << endl;

        wg.create_graph(words, true);
        if (!no_tying) wg.tie_graph(true, remove_cw_markers);

        wg.add_silence_loop(sentence_end_symbol);
        wg.add_hmm_self_transitions();

        wg.write_graph(graphfname, word_labels);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
