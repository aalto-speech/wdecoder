#include <iostream>
#include <string>
#include <ctime>
#include <climits>
#include <sstream>

#include "Decoder.hh"
#include "Lookahead.hh"
#include "conf.hh"

using namespace std;


void
read_config(Decoder &d, string cfgfname)
{
    ifstream cfgf(cfgfname);
    if (!cfgf) throw string("Problem opening configuration file: ") + cfgfname;

    string line;
    while (getline(cfgf, line)) {
        if (!line.length()) continue;
        stringstream ss(line);
        string parameter, val;
        ss >> parameter;
        if (parameter == "lm_scale") ss >> d.m_lm_scale;
        else if (parameter == "token_limit") ss >> d.m_token_limit;
        else if (parameter == "node_limit") ss >> d.m_active_node_limit;
        else if (parameter == "duration_scale") ss >> d.m_duration_scale;
        else if (parameter == "transition_scale") ss >> d.m_transition_scale;
        else if (parameter == "global_beam") ss >> d.m_global_beam;
        else if (parameter == "acoustic_beam") continue;
        else if (parameter == "history_beam") continue;
        else if (parameter == "word_end_beam") ss >> d.m_word_end_beam;
        else if (parameter == "node_beam") ss >> d.m_node_beam;
        else if (parameter == "word_boundary_penalty") ss >> d.m_word_boundary_penalty;
        else if (parameter == "history_clean_frame_interval") ss >> d.m_history_clean_frame_interval;
        else if (parameter == "force_sentence_end") {
            string force_str;
            ss >> force_str;
            d.m_force_sentence_end = true ? force_str == "true": false;
        }
        else if (parameter == "word_boundary_symbol") {
            d.m_use_word_boundary_symbol = true;
            ss >> d.m_word_boundary_symbol;
            d.m_word_boundary_symbol_idx = d.m_subword_map[d.m_word_boundary_symbol];
        }
        else if (parameter == "debug") ss >> d.m_debug;
        else if (parameter == "stats") ss >> d.m_stats;
        else if (parameter == "token_stats") ss >> d.m_token_stats;
        else throw string("Unknown parameter: ") + parameter;
    }

    cfgf.close();
}


void
print_config(Decoder &d,
             conf::Config &config,
             ostream &outf)
{
    outf << "PH: " << config.arguments[0] << endl;
    outf << "LEXICON: " << config.arguments[1] << endl;
    outf << "LM: " << config.arguments[2] << endl;
    outf << "GRAPH: " << config.arguments[4] << endl;

    outf << std::boolalpha;
    outf << "lm scale: " << d.m_lm_scale << endl;
    outf << "active node limit: " << d.m_active_node_limit << endl;
    outf << "token limit: " << d.m_token_limit << endl;
    outf << "duration scale: " << d.m_duration_scale << endl;
    outf << "transition scale: " << d.m_transition_scale << endl;
    outf << "force sentence end: " << d.m_force_sentence_end << endl;
    outf << "use word boundary symbol: " << d.m_use_word_boundary_symbol << endl;
    if (d.m_use_word_boundary_symbol)
        outf << "word boundary symbol: " << d.m_word_boundary_symbol << endl;
    outf << "global beam: " << d.m_global_beam << endl;
    outf << "word end beam: " << d.m_word_end_beam << endl;
    outf << "node beam: " << d.m_node_beam << endl;
    outf << "word boundary penalty: " << d.m_word_boundary_penalty << endl;
    outf << "history clean frame interval: " << d.m_history_clean_frame_interval << endl;
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: score [OPTION...] PH LEXICON LM CFGFILE GRAPH LNALIST RESLIST\n")
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
        read_config(d, cfgfname);

        string graphfname = config.arguments[4];
        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);
        cerr << "node count: " << d.m_nodes.size() << endl;

        d.m_la = new NoLookahead(d);

        print_config(d, config, cout);
        cout << endl;

        string lnalistfname = config.arguments[5];
        ifstream lnalistf(lnalistfname);
        string line;

        string reslistfname = config.arguments[6];
        ifstream reslistf(reslistfname);
        string resline;

        int total_frames = 0;
        double total_time = 0.0;
        double total_lp = 0.0;
        int file_count = 0;

        while (getline(lnalistf, line)) {
            getline(reslistf, resline);
            if (!line.length()) continue;
            cerr << "scoring: " << line << endl;

            vector<string> reswordstrs;
            vector<int> reswords;
            stringstream ress(resline);
            string tempstr;
            while (ress >> tempstr) {
                reswordstrs.push_back(tempstr);
                reswords.push_back(d.m_subword_map[tempstr]);
            }

            std::vector<std::vector<int> > paths;
            d.find_paths(paths, reswords, 2);
            double best_lp = -1e20, best_am_lp = -1e20, best_lm_lp = -1e20;
            double worst_lp = 1e20, worst_am_lp = 1e20, worst_lm_lp = 1e20;
            int curr_frames;
            for (auto pit = paths.begin(); pit != paths.end(); ++pit) {
                vector<Decoder::Node> nodes;
                d.path_to_graph(*pit, nodes);

                d.m_nodes.swap(nodes);
                d.set_hmm_transition_probs();
                int original_decode_start_node = d.m_decode_start_node;
                d.m_decode_start_node = 0;

                double curr_time, curr_lp, curr_am_lp, curr_lm_lp;
                if (pit == paths.begin())
                    d.recognize_lna_file(line, cout, &curr_frames, &curr_time,
                                         &curr_lp, &curr_am_lp, &curr_lm_lp);
                else {
                    stringstream tmp;
                    d.recognize_lna_file(line, tmp, &curr_frames, &curr_time,
                                         &curr_lp, &curr_am_lp, &curr_lm_lp);
                }

                d.m_decode_start_node = original_decode_start_node;
                nodes.swap(d.m_nodes);

                if (curr_lp > best_lp) {
                    best_lp = curr_lp;
                    best_am_lp = curr_am_lp;
                    best_lm_lp = curr_lm_lp;
                }
                if (curr_lp < worst_lp) {
                    worst_lp = curr_lp;
                    worst_am_lp = curr_am_lp;
                    worst_lm_lp = curr_lm_lp;
                }
            }

            cerr << "\tnumber of alternative paths: " << paths.size() << endl;
            cerr << "\tframes: " << curr_frames << endl;
            cerr << "\tBest log prob: " << best_lp << "\tAM: " << best_am_lp << "\tLM: " << best_lm_lp << endl;
            cerr << "\tWorst log prob: " << worst_lp << "\tAM: " << worst_am_lp << "\tLM: " << worst_lm_lp << endl;

            total_frames += curr_frames;
            total_lp += best_lp;
            file_count++;
        }
        lnalistf.close();

        cerr << endl;
        cerr << file_count << " files scored" << endl;
        cerr << "total frame count: " << total_frames << endl;
        cerr << "total log prob: " << total_lp << endl;

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
