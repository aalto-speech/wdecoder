#include <sstream>
#include <thread>

#include "NgramDecoder.hh"
#include "Lookahead.hh"
#include "ClassLookahead.hh"
#include "conf.hh"
#include "str.hh"

using namespace std;


void
read_config(NgramDecoder &d, string cfgfname)
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
            d.m_use_word_boundary_symbol = true;
            ss >> d.m_word_boundary_symbol;
        }
        else if (parameter == "stats") ss >> d.m_stats;
        else throw string("Unknown parameter: ") + parameter;
    }

    cfgf.close();
}


void
print_config(NgramDecoder &d,
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
    outf << "number of threads: " << config["num-threads"].get_int() << endl;

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
}


void join(vector<string> &lnafnames,
          vector<NgramRecognition*> &recognitions,
          vector<RecognitionResult*> &results,
          vector<thread*> &threads,
          RecognitionResult &total,
          ostream &resultf,
          ostream &logf)
{
    for (int i=0; i<(int)threads.size(); i += 1) {
        threads[i]->join();
        logf << endl << "recognizing: " << lnafnames[i] << endl;
        resultf << lnafnames[i] << ":" << results[i]->result << endl;
        results[i]->print_file_stats(logf);
        total.accumulate(*results[i]);
        delete recognitions[i];
        delete results[i];
        delete threads[i];
    }
    lnafnames.clear();
    recognitions.clear();
    results.clear();
    threads.clear();
}


void
recognize_lnas(NgramDecoder &d,
               conf::Config &config,
               string lnalistfname,
               ostream &resultf,
               ostream &logf)
{
    ifstream lnalistf(lnalistfname);
    string lnafname;
    RecognitionResult total;

    print_config(d, config, logf);

    int file_count = 0;
    int num_threads = config["num-threads"].get_int();
    vector<string> lna_fnames;
    vector<NgramRecognition*> recognitions;
    vector<RecognitionResult*> results;
    vector<std::thread*> threads;
    while (getline(lnalistf, lnafname)) {
        if (!lnafname.length()) continue;

        if ((int)recognitions.size() < num_threads) {
            lna_fnames.push_back(lnafname);
            recognitions.push_back(new NgramRecognition(d));
            results.push_back(new RecognitionResult());
            thread *thr = new thread(&NgramRecognition::recognize_lna_file,
                                     recognitions.back(),
                                     lnafname,
                                     std::ref(*results.back()));
            threads.push_back(thr);
        }

        if ((int)recognitions.size() == num_threads)
            join(lna_fnames, recognitions, results, threads, total, resultf, logf);

        file_count++;
    }
    lnalistf.close();
    join(lna_fnames, recognitions, results, threads, total, resultf, logf);
    if (file_count > 1) {
        logf << endl;
        logf << file_count << " files recognized" << endl;
        total.print_final_stats(logf);
    }
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: decode [OPTION...] PH LEXICON LM CFGFILE GRAPH LNALIST\n")
    ('h', "help", "", "", "display help")
    ('d', "duration-model=STRING", "arg", "", "Duration model")
    ('q', "quantized-lookahead", "", "", "Two byte quantized look-ahead model")
    ('p', "num-threads", "arg", "1", "Number of threads")
    ('f', "result-file=STRING", "arg", "", "Base filename for results (.rec and .log)")
    ('l', "lookahead-model=STRING", "arg", "", "Lookahead language model")
    ('t', "lookahead-type=STRING", "arg", "", "Lookahead type\n"
            "\tunigram\n"
            "\tclass-bigram\n"
            "\tbigram-full\n"
            "\tbigram-precomputed-full\n"
            "\tbigram-hybrid\n"
            "\tbigram-precomputed-hybrid\n"
            "\tlarge-bigram")
    ('w', "write-la-states=STRING", "arg", "", "Writes lookahead model information to a file");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 6) config.print_help(stderr, 1);

    try {
        NgramDecoder d;

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

        // Read before loading language model
        // for possible word boundary symbol configuration
        string cfgfname = config.arguments[3];
        cerr << "Reading configuration: " << cfgfname << endl;
        read_config(d, cfgfname);

        string lmfname = config.arguments[2];
        cerr << "Reading language model: " << lmfname << endl;
        d.read_lm(lmfname);

        string graphfname = config.arguments[4];
        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);
        cerr << "node count: " << d.m_nodes.size() << endl;

        if (config["lookahead-model"].specified) {
            string la_type = config["lookahead-type"].get_str();
            string lalmfname = config["lookahead-model"].get_str();
            cerr << "Reading lookahead model: " << lalmfname << endl;

            bool quantization = config["quantized-lookahead"].specified;
            if (la_type == "unigram")
                d.m_la = new UnigramLookahead(d, lalmfname);
            else if (la_type == "class-bigram") {
                vector<string> class_la_model = str::split(lalmfname, ",", false);
                d.m_la = new ClassBigramLookahead(d, class_la_model[0], class_la_model[1], true);
            }
            else if (la_type == "bigram-full")
                d.m_la = new FullTableBigramLookahead(d, lalmfname);
            else if (la_type == "bigram-precomputed-full")
                d.m_la = new PrecomputedFullTableBigramLookahead(d, lalmfname, quantization);
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

        string lnalistfname = config.arguments[5];

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
