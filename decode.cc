#include <iostream>
#include <string>
#include <ctime>
#include <climits>

#include "Decoder.hh"
#include "LM.hh"
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
    config("usage: decode [OPTION...] PH LEXICON LM CFGFILE GRAPH LNALIST\n")
      ('h', "help", "", "", "display help")
      ('d', "duration-model=STRING", "arg", "", "Duration model")
      ('u', "unigram-lookahead=STRING", "arg", "", "Unigram lookahead language model");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 6) config.print_help(stderr, 1);

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

        if (config["unigram-lookahead"].specified) {
            string lalmfname = config["unigram-lookahead"].get_str();
            cerr << "Reading unigram lookahead model: " << lalmfname << endl;
            d.read_la_lm(lalmfname);
        }

        d.write_bigram_la_scores("bigram-la-scores.txt");

        string lnalistfname = config.arguments[5];
        ifstream lnalistf(lnalistfname);
        string line;

        int total_frames = 0;
        double total_time = 0.0;
        double total_lp = 0.0;
        int file_count = 0;
        while (getline(lnalistf, line)) {
            if (!line.length()) continue;
            cerr << endl << "recognizing: " << line << endl;
            int curr_frames;
            double curr_time;
            double curr_lp, curr_am_lp, curr_lm_lp;
            d.recognize_lna_file(line, cout, &curr_frames, &curr_time,
                                 &curr_lp, &curr_am_lp, &curr_lm_lp);
            total_frames += curr_frames;
            total_time += curr_time;
            total_lp += curr_lp;
            cerr << "\trecognized " << curr_frames << " frames in " << curr_time << " seconds." << endl;
            cerr << "\tRTF: " << curr_time / ((double)curr_frames/125.0) << endl;
            cerr << "\tLog prob: " << curr_lp << "\tAM: " << curr_am_lp << "\tLM: " << curr_lm_lp << endl;
            file_count++;
        }
        lnalistf.close();

        if (file_count > 1) {
            cerr << endl;
            cerr << file_count << " files recognized" << endl;
            cerr << "total recognition time: " << total_time << endl;
            cerr << "total frame count: " << total_frames << endl;
            cerr << "total RTF: " << total_time/ ((double)total_frames/125.0) << endl;
            cerr << "total log prob: " << total_lp << endl;
        }

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
