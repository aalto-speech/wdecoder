#include <iostream>
#include <string>
#include <ctime>
#include <climits>
#include <sstream>

#include "Decoder.hh"
#include "conf.hh"
#include "str.hh"

using namespace std;


vector<float> lm_scales;
vector<float> beams;


void parse_csv(string valstr, vector<float> &values)
{
    values.clear();
    vector<string> fields;
    str::split_with_quotes(&valstr, " ,", true, &fields);
    for (unsigned int i=0; i<fields.size(); i++)
        values.push_back(stof(fields[i]));
}


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
        if (parameter == "lm_scale") {
            string lmscale_csv;
            ss >> lmscale_csv;
            parse_csv(lmscale_csv, lm_scales);
        }
        else if (parameter == "token_limit") ss >> d.m_token_limit;
        else if (parameter == "node_limit") ss >> d.m_active_node_limit;
        else if (parameter == "duration_scale") ss >> d.m_duration_scale;
        else if (parameter == "transition_scale") ss >> d.m_transition_scale;
        else if (parameter == "global_beam") {
            string global_beam_csv;
            ss >> global_beam_csv;
            parse_csv(global_beam_csv, beams);
        }
        else if (parameter == "acoustic_beam") ss >> d.m_acoustic_beam;
        else if (parameter == "history_beam") ss >> d.m_history_beam;
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
        else if (parameter == "branching_stats") ss >> d.m_branching_stats;
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
    if (config["lookahead-model"].specified) {
        string lalmfname = config["lookahead-model"].get_str();
        outf << "LOOKAHEAD LM: " << lalmfname << endl;
    }

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
    outf << "acoustic global beam: " << d.m_acoustic_beam << endl;
    outf << "acoustic history beam: " << d.m_history_beam << endl;
    outf << "word end beam: " << d.m_word_end_beam << endl;
    outf << "node beam: " << d.m_node_beam << endl;
    outf << "word boundary penalty: " << d.m_word_boundary_penalty << endl;
    outf << "history clean frame interval: " << d.m_history_clean_frame_interval << endl;
}


void
recognize_lnas(Decoder &d,
               conf::Config &config,
               string lnalistfname,
               ostream &resultf,
               ostream &logf,
               float lm_scale,
               float global_beam,
               float global_ac_beam,
               float word_end_beam)
{
    ifstream lnalistf(lnalistfname);
    string line;

    d.m_lm_scale = lm_scale;
    d.m_global_beam = global_beam;
    d.m_acoustic_beam = global_ac_beam;
    d.m_word_end_beam = word_end_beam;
    print_config(d, config, logf);

    int total_frames = 0;
    double total_time = 0.0;
    double total_lp = 0.0;
    int file_count = 0;
    while (getline(lnalistf, line)) {
        if (!line.length()) continue;
        logf << endl << "recognizing: " << line << endl;
        int curr_frames;
        double curr_time;
        double curr_lp, curr_am_lp, curr_lm_lp;
        double propagation_ratio;
        d.recognize_lna_file(line, resultf, &curr_frames, &curr_time,
                             &curr_lp, &curr_am_lp, &curr_lm_lp, &propagation_ratio);
        total_frames += curr_frames;
        total_time += curr_time;
        total_lp += curr_lp;
        logf << "\trecognized " << curr_frames << " frames in " << curr_time << " seconds." << endl;
        logf << "\tRTF: " << curr_time / ((double)curr_frames/125.0) << endl;
        logf << "\tLog prob: " << curr_lp << "\tAM: " << curr_am_lp << "\tLM: " << curr_lm_lp << endl;
        if (d.m_branching_stats)
            logf << "\tToken propagation ratio: " << propagation_ratio << endl;
        file_count++;
    }
    lnalistf.close();

    if (file_count > 1) {
        logf << endl;
        logf << file_count << " files recognized" << endl;
        logf << "total recognition time: " << total_time << endl;
        logf << "total frame count: " << total_frames << endl;
        logf << "total RTF: " << total_time/ ((double)total_frames/125.0) << endl;
        logf << "total log prob: " << total_lp << endl;
    }
}



int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: decode [OPTION...] PH LEXICON LM CFGFILE GRAPH LNALIST\n")
    ('h', "help", "", "", "display help")
    ('d', "duration-model=STRING", "arg", "", "Duration model")
    ('f', "result-file=STRING", "arg", "", "Base filename for results (.rec and .log)")
    ('l', "lookahead-model=STRING", "arg", "", "Lookahead language model")
    ('t', "lookahead-tables", "", "", "Set bigram lookahead tables");
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
        read_config(d, cfgfname);

        string graphfname = config.arguments[4];
        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);
        cerr << "node count: " << d.m_nodes.size() << endl;

        if (config["lookahead-model"].specified) {
            bool la_tables = config["lookahead-tables"].specified;
            string lalmfname = config["lookahead-model"].get_str();
            cerr << "Reading lookahead model: " << lalmfname << endl;
            d.read_la_lm(lalmfname, la_tables);
        }

        string lnalistfname = config.arguments[5];

        if (lm_scales.size() == 0) {
            cerr << "No lm scales set." << endl;
            exit(0);
        }

        if (lm_scales.size() != beams.size()) {
            cerr << "Number of set lm scales differs from set beams." << endl;
            exit(0);
        }

        if (d.m_branching_stats) {
            cerr << "Computing branching counts for each HMM node." << endl;
            d.m_branching_counts.resize(d.m_nodes.size());
            for (int i=0; i<d.m_nodes.size(); i++) {
                if (d.m_nodes[i].hmm_state == -1) continue;
                set<int> successor_hmm_nodes;
                d.find_successor_hmm_nodes(i, successor_hmm_nodes);
                d.m_branching_counts[i] = successor_hmm_nodes.size();
            }
        }

        if (config["result-file"].specified) {
            string resultfname = config["result-file"].get_str();
            cerr << "Base filename for results: " << resultfname << endl;
            for (unsigned int i=0; i<lm_scales.size(); ++i) {
                string curr_resfname = resultfname + string(".lmscale") + to_string(int(lm_scales[i]))
                        + string(".beam") + to_string(int(beams[i])) + string(".rec");
                string curr_logfname = resultfname + string(".lmscale") + to_string(int(lm_scales[i]))
                        + string(".beam") + to_string(int(beams[i])) + string(".log");
                ofstream resultf(curr_resfname);
                ofstream logf(curr_logfname);
                recognize_lnas(d,
                               config,
                               lnalistfname,
                               resultf,
                               logf,
                               lm_scales[i],
                               beams[i],
                               beams[i]-40.0,
                               (2.0/3.0) * beams[i]);
            }
        }
        else {
            for (unsigned int i=0; i<lm_scales.size(); ++i)
                recognize_lnas(d,
                               config,
                               lnalistfname,
                               cout,
                               cerr,
                               lm_scales[i],
                               beams[i],
                               d.m_acoustic_beam,
                               d.m_word_end_beam);
        }

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(1);
}
