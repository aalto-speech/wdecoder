#include <iostream>
#include <string>
#include <ctime>
#include <climits>
#include <sstream>

#include "Decoder.hh"
#include "conf.hh"

using namespace std;


void print_graph(Decoder &d, string fname) {
    ofstream outf(fname);
    d.print_dot_digraph(d.m_nodes, outf);
    outf.close();
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: decode [OPTION...] PH LEXICON LM CFGFILE GRAPH LNALIST RESLIST\n")
      ('h', "help", "", "", "display help")
      ('d', "duration-model=STRING", "arg", "", "Duration model");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 7) config.print_help(stderr, 1);

    try {

        Decoder d;

        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        d.read_phone_model(phfname);

        if (config["duration-model"].specified) {
            string durfname = config["duration-model"].get_str();
            cerr << "Reading duration model: " << durfname << endl;
            d.read_duration_model(durfname);
        }

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        d.read_noway_lexicon(lexfname);

        string lmfname = config.arguments[2];
        cerr << "Reading language model: " << lmfname << endl;
        d.read_lm(lmfname);

        string cfgfname = config.arguments[3];
        cerr << "Reading configuration: " << cfgfname << endl;
        d.read_config(cfgfname);
        d.print_config(cerr);

        string graphfname = config.arguments[4];
        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);
        cerr << "node count: " << d.m_nodes.size() << endl;

        string lnalistfname = config.arguments[5];
        ifstream lnalistf(lnalistfname);
        string line;

        string reslistfname = config.arguments[6];
        ifstream reslistf(reslistfname);
        string resline;

        while (getline(lnalistf, line)) {
            getline(reslistf, resline);
            if (!line.length()) continue;
            cerr << "scoring file: " << line << endl;
            cerr << "result: " << resline << endl;

            vector<string> reswordstrs;
            vector<int> reswords;
            stringstream ress(resline);
            string tempstr;
            while (ress >> tempstr) {
                reswordstrs.push_back(tempstr);
                reswords.push_back(d.m_subword_map[tempstr]);
                cerr << tempstr << " (" << d.m_subword_map[tempstr] << ") ";
            }
            cerr << endl;

            std::vector<std::vector<int> > paths;
            d.find_paths(paths, reswords, 2);
            for (auto pit = paths.begin(); pit != paths.end(); ++pit) {
                vector<Decoder::Node> nodes;
                d.path_to_graph(*pit, nodes);

                /*
                for (int ni = 0; ni < nodes.size(); ni++) {
                    cerr << nodes[ni].hmm_state;
                    if (nodes[ni].word_id != -1) cerr << " (" << d.m_subwords[nodes[ni].word_id] << ")";
                    cerr << endl;
                }
                */

                d.m_nodes.swap(nodes);
                d.set_hmm_transition_probs(d.m_nodes);
                int original_decode_start_node = d.DECODE_START_NODE;
                d.DECODE_START_NODE = 0;

                int curr_frames;
                double curr_time;
                double curr_lp, curr_am_lp, curr_lm_lp;
                d.recognize_lna_file(line, cout, &curr_frames, &curr_time,
                                     &curr_lp, &curr_am_lp, &curr_lm_lp);
                cerr << "\tscored " << curr_frames << " frames in " << curr_time << " seconds." << endl;
                cerr << "\tRTF: " << curr_time / ((double)curr_frames/125.0) << endl;
                cerr << "\tLog prob: " << curr_lp << "\tAM: " << curr_am_lp << "\tLM: " << curr_lm_lp << endl;

                d.DECODE_START_NODE = original_decode_start_node;
                nodes.swap(d.m_nodes);
            }
            cerr << "number of alternative paths: " << paths.size() << endl;
        }
        lnalistf.close();


    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
