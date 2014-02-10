#include <iostream>
#include <string>
#include <ctime>
#include <climits>

#include "Decoder.hh"
#include "LM.hh"
#include "conf.hh"

using namespace std;


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: dgraph [OPTION...] PH DUR LEXICON LM GRAPH LNALIST\n")
      ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 6) config.print_help(stderr, 1);

    try {

        Decoder d;

        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        d.read_phone_model(phfname);

        string durfname = config.arguments[1];
        cerr << "Reading duration models: " << durfname << endl;
        d.read_duration_model(durfname);

        string lexfname = config.arguments[2];
        cerr << "Reading lexicon: " << lexfname << endl;
        d.read_noway_lexicon(lexfname);

        string lmfname = config.arguments[3];
        cerr << "Reading language model: " << lmfname << endl;
        d.read_lm(lmfname);

        string graphfname = config.arguments[4];
        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);
        cerr << "node count: " << d.m_nodes.size() << endl;

//        ofstream outf("graph.dot");
//        d.print_dot_digraph(d.m_nodes, outf);
//        outf.close();
//        exit(1);

        d.set_lm_scale(34.0);
        d.set_duration_scale(3.0);
        d.set_transition_scale(1.0);
        d.set_global_beam(200.0);
        d.set_word_end_beam(120.0);
        d.set_history_beam(140.0);
        d.set_state_beam(150.0);
        d.set_silence_beam(120.0);
        d.set_history_limit(70);
        d.set_word_boundary_penalty(-0.01);
        d.set_force_sentence_end(true);

        d.debug=0;
        d.stats=0;

        string lnalistfname = config.arguments[5];
        ifstream lnalistf(lnalistfname);
        string line;

        while (getline(lnalistf, line)) {
            if (!line.length()) continue;
            cerr << "recognizing: " << line << endl;
            d.recognize_lna_file(line);
        }
        lnalistf.close();

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
