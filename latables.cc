#include <iostream>
#include <string>
#include <ctime>
#include <climits>

#include "Decoder.hh"
#include "LM.hh"
#include "conf.hh"
#include "io.hh"

using namespace std;


void print_graph(Decoder &d, string fname) {
    ofstream outf(fname);
    d.print_dot_digraph(d.m_nodes, outf);
    outf.close();
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: decode [OPTION...] PH LEXICON CFGFILE GRAPH LALM LATABLES\n")
      ('h', "help", "", "", "display help")
      ('d', "initial-depth=INT", "arg", "2", "Depth for initial nodes")
      ('c', "cross-word", "", "", "Create lookahead tables for all cross-word nodes");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 6) config.print_help(stderr, 1);

    try {

        Decoder d;
        d.m_initial_node_depth = config["initial-depth"].get_int();

        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        d.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        d.read_noway_lexicon(lexfname);

        string cfgfname = config.arguments[2];
        cerr << "Reading configuration: " << cfgfname << endl;
        d.read_config(cfgfname);

        string graphfname = config.arguments[3];
        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);
        cerr << "node count: " << d.m_nodes.size() << endl;

        string lalmfname = config.arguments[4];
        cerr << "Reading lookahead model: " << lalmfname << endl;

        d.m_la_lm.read_arpa(io::Stream(lalmfname, "r").file, true);
        d.m_la_lm.trim();
        d.set_subword_id_la_fsa_symbol_mapping();
        if (d.m_la_lm.order() != 3) {
            cerr << "Lookahead language model should be trigram" << endl;
            exit(1);
        }

        d.m_debug = true;
        cerr << "Creating lookahead tables" << endl;
        // fan_out_dummy, fan_in_dummy, initial, silence, all_cw
        bool all_cw = config["cross-word"].specified;
        d.create_la_tables(true, true, true, true, all_cw);
        //d.create_bigram_la_tables(true, true, true, true, all_cw);

        string latfname = config.arguments[5];
        cerr << "Writing precomputed lookahead tables: " << latfname << endl;
        d.write_bigram_la_tables(latfname);

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
