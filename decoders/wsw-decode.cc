#include <sstream>

#include "WordSubwordDecoder.hh"
#include "Lookahead.hh"
#include "conf.hh"

using namespace std;


void
read_config(WordSubwordDecoder &d, string cfgfname)
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
        else if (parameter == "duration_scale") ss >> d.m_duration_scale;
        else if (parameter == "transition_scale") ss >> d.m_transition_scale;
        else if (parameter == "global_beam") ss >> d.m_global_beam;
        else if (parameter == "word_end_beam") ss >> d.m_word_end_beam;
        else if (parameter == "node_beam") ss >> d.m_node_beam;
        else if (parameter == "history_clean_frame_interval") ss >> d.m_history_clean_frame_interval;
        else if (parameter == "force_sentence_end") {
            string force_str;
            ss >> force_str;
            d.m_force_sentence_end = (force_str == "true");
        }
        else if (parameter == "word_boundary_symbol") {
            cerr << "FIXME, word boundary symbol" << endl;
            exit(1);
            d.m_use_word_boundary_symbol = true;
            ss >> d.m_word_boundary_symbol;
            //d.m_word_boundary_symbol_idx = d.m_word_map[d.m_word_boundary_symbol];
        }
        else if (parameter == "stats") ss >> d.m_stats;
        else if (parameter == "word_iw") ss >> d.m_word_iw;
        else if (parameter == "class_iw") ss >> d.m_class_iw;
        else if (parameter == "subword_iw") ss >> d.m_subword_iw;
        else throw string("Unknown parameter: ") + parameter;
    }

    if (d.m_word_iw+d.m_class_iw+d.m_subword_iw != 1.0) {
        cerr << "Invalid interpolation weights," << endl;
        cerr << "word_iw: " << d.m_word_iw << endl;
        cerr << "class_iw: " << d.m_class_iw << endl;
        cerr << "subword_iw: " << d.m_subword_iw << endl;
    }
    else {
        d.m_word_iw = log(d.m_word_iw);
        d.m_class_iw = log(d.m_class_iw);
        d.m_subword_iw = log(d.m_subword_iw);
    }

    cfgf.close();
}


void
print_config(WordSubwordDecoder &d,
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
    outf << "history clean frame interval: " << d.m_history_clean_frame_interval << endl;
    outf << "interpolation weight (word lm): " << exp(d.m_word_iw) << endl;
    outf << "interpolation weight (class lm): " << exp(d.m_class_iw) << endl;
    outf << "interpolation weight (subword lm): " << exp(d.m_subword_iw) << endl;
}


void
recognize_lnas(WordSubwordDecoder &d,
               conf::Config &config,
               string lnalistfname,
               ostream &resultf,
               ostream &logf)
{
    ifstream lnalistf(lnalistfname);
    string line;

    print_config(d, config, logf);

    int total_frames = 0;
    double total_time = 0.0;
    double total_lp = 0.0;
    double total_am_lp = 0.0;
    double total_lm_lp = 0.0;
    double total_token_count = 0.0;
    int file_count = 0;
    while (getline(lnalistf, line)) {
        if (!line.length()) continue;
        logf << endl << "recognizing: " << line << endl;
        int curr_frames;
        double curr_time;
        double curr_lp, curr_am_lp, curr_lm_lp;
        double token_count;
        d.recognize_lna_file(line, resultf, &curr_frames, &curr_time,
                             &curr_lp, &curr_am_lp, &curr_lm_lp, &token_count);
        total_frames += curr_frames;
        total_time += curr_time;
        total_lp += curr_lp;
        total_am_lp += curr_am_lp;
        total_lm_lp += curr_lm_lp;
        total_token_count += token_count;
        logf << "\trecognized " << curr_frames << " frames in " << curr_time << " seconds." << endl;
        logf << "\tRTF: " << curr_time / ((double)curr_frames/125.0) << endl;
        logf << "\tLog prob: " << curr_lp << "\tAM: " << curr_am_lp << "\tLM: " << curr_lm_lp << endl;
        logf << "\tMean token count: " << token_count / (double)curr_frames << endl;
        file_count++;
    }
    lnalistf.close();

    if (file_count > 1) {
        logf << endl;
        logf << file_count << " files recognized" << endl;
        logf << "total recognition time: " << total_time << endl;
        logf << "total frame count: " << total_frames << endl;
        logf << "total RTF: " << total_time/ ((double)total_frames/125.0) << endl;
        logf << "total log likelihood: " << total_lp << endl;
        logf << "total LM likelihood: " << total_lm_lp << endl;
        logf << "total AM likelihood: " << total_am_lp << endl;
        logf << "total mean token count: " << total_token_count / (double)total_frames << endl;
    }
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: wsw-decode [OPTION...] PH LEXICON WORD_ARPA CLASS_ARPA CMEMPROBS SUBWORD_NGRAM SUBWORD_SEGS CFGFILE GRAPH LNALIST\n")
    ('h', "help", "", "", "display help")
    ('d', "duration-model=STRING", "arg", "", "Duration model")
    ('f', "result-file=STRING", "arg", "", "Base filename for results (.rec and .log)")
    ('l', "lookahead-model=STRING", "arg", "", "Lookahead language model")
    ('t', "lookahead-type=STRING", "arg", "", "Lookahead type\n"
            "\tunigram\n"
            "\tbigram-full\n"
            "\tbigram-precomputed-full\n"
            "\tbigram-hybrid\n"
            "\tbigram-precomputed-hybrid\n"
            "\tlarge-bigram")
    ('w', "write-la-states=STRING", "arg", "", "Writes lookahead model information to a file");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 10) config.print_help(stderr, 1);

    try {

        WordSubwordDecoder d;

        string phfname = config.arguments[0];
        string lexfname = config.arguments[1];
        string lmfname = config.arguments[2];
        string class_arpa_fname = config.arguments[3];
        string wordpfname = config.arguments[4];
        string subword_arpa_fname = config.arguments[5];
        string subword_seg_fname = config.arguments[6];
        string cfgfname = config.arguments[7];
        string graphfname = config.arguments[8];
        string lnalistfname = config.arguments[9];

        cerr << "Reading hmms: " << phfname << endl;
        d.read_phone_model(phfname);

        if (config["duration-model"].specified) {
            string durfname = config["duration-model"].get_str();
            cerr << "Reading duration model: " << durfname << endl;
            d.read_duration_model(durfname);
        }

        cerr << "Reading lexicon: " << lexfname << endl;
        d.read_noway_lexicon(lexfname);

        cerr << "Reading language model: " << lmfname << endl;
        d.read_lm(lmfname);

        cerr << "Reading configuration: " << cfgfname << endl;
        read_config(d, cfgfname);

        d.read_class_lm(class_arpa_fname, wordpfname);

        d.read_subword_lm(subword_arpa_fname, subword_seg_fname);

        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);
        cerr << "node count: " << d.m_nodes.size() << endl;

        if (config["lookahead-model"].specified) {
            string la_type = config["lookahead-type"].get_str();
            string lalmfname = config["lookahead-model"].get_str();
            cerr << "Reading lookahead model: " << lalmfname << endl;

            if (la_type == "unigram")
                d.m_la = new UnigramLookahead(d, lalmfname);
            else if (la_type == "bigram-full")
                d.m_la = new FullTableBigramLookahead(d, lalmfname);
            else if (la_type == "bigram-precomputed-full")
                d.m_la = new PrecomputedFullTableBigramLookahead(d, lalmfname);
            else if (la_type == "bigram-hybrid")
                d.m_la = new HybridBigramLookahead(d, lalmfname);
            else if (la_type == "bigram-precomputed-hybrid")
                d.m_la = new PrecomputedHybridBigramLookahead(d, lalmfname);
            else if (la_type == "large-bigram")
                d.m_la = new LargeBigramLookahead(d, lalmfname);
            else {
                cerr << "unknown lookahead type: " << la_type << endl;
                exit(1);
            }
        }

        if (config["write-la-states"].specified) {
            string lasfname = config["write-la-states"].get_str();
            cerr << "Writing lookahead state information: " << lasfname << endl;
            d.m_la->write(lasfname);
        }

        if (config["result-file"].specified) {
            string resultfname = config["result-file"].get_str();
            cerr << "Base filename for results: " << resultfname << endl;
            string resfname = resultfname + string(".rec");
            string logfname = resultfname + string(".log");
            ofstream resultf(resfname);
            ofstream logf(logfname);
            recognize_lnas(d, config, lnalistfname, resultf, logf);
        }
        else {
            recognize_lnas(d, config, lnalistfname, cout, cerr);
        }

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}