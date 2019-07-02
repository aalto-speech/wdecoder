#include <sstream>
#include <thread>

#include "WordSubwordDecoder.hh"
#include "Lookahead.hh"
#include "ClassLookahead.hh"
#include "conf.hh"
#include "str.hh"

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
        else cerr << "Ignored unknown parameter: " << parameter << endl;
    }

    if (fabs(d.m_word_iw+d.m_class_iw+d.m_subword_iw - 1.0) > 0.00001) {
        cerr << "Invalid interpolation weights," << endl;
        cerr << "word_iw: " << d.m_word_iw << endl;
        cerr << "class_iw: " << d.m_class_iw << endl;
        cerr << "subword_iw: " << d.m_subword_iw << endl;
        exit(1);
    } else {
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
    outf << "interpolation weight (word lm): " << exp(d.m_word_iw) << endl;
    outf << "interpolation weight (class lm): " << exp(d.m_class_iw) << endl;
    outf << "interpolation weight (subword lm): " << exp(d.m_subword_iw) << endl;
}


void
join(vector<string> &lnafnames,
     vector<WordSubwordRecognition*> &recognitions,
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
recognize_lnas(WordSubwordDecoder &d,
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
    vector<WordSubwordRecognition*> recognitions;
    vector<RecognitionResult*> results;
    vector<std::thread*> threads;
    while (getline(lnalistf, lnafname)) {
        if (!lnafname.length()) continue;

        if ((int)recognitions.size() < num_threads) {
            lna_fnames.push_back(lnafname);
            recognitions.push_back(new WordSubwordRecognition(d));
            results.push_back(new RecognitionResult());
            thread *thr = new thread(&WordSubwordRecognition::recognize_lna_file,
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
    config("usage: wsw-decode [OPTION...] PH LEXICON WORD_ARPA CLASS_ARPA CMEMPROBS SUBWORD_NGRAM SUBWORD_SEGS CFGFILE GRAPH LNALIST\n")
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
            "\tlarge-bigram");
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

            bool quantization = config["quantized-lookahead"].specified;
            if (la_type == "unigram")
                d.m_la = new UnigramLookahead(d, lalmfname);
            else if (la_type == "class-bigram") {
                vector<string> class_la_model = str::split(lalmfname, ",", false);
                d.m_la = new ClassBigramLookahead(d, class_la_model[0], class_la_model[1], quantization);
            }
            else if (la_type == "bigram-full")
                d.m_la = new FullTableBigramLookahead(d, lalmfname);
            else if (la_type == "bigram-precomputed-full")
                d.m_la = new PrecomputedFullTableBigramLookahead(d, lalmfname, quantization);
            else if (la_type == "bigram-hybrid")
                d.m_la = new HybridBigramLookahead(d, lalmfname);
            else if (la_type == "bigram-precomputed-hybrid")
                d.m_la = new PrecomputedHybridBigramLookahead(d, lalmfname);
            else if (la_type == "large-bigram") {
                vector<string> bigram_la_model = str::split(lalmfname, ",", false);
                if (bigram_la_model.size() > 1)
                    d.m_la = new LargeBigramLookahead(d, bigram_la_model[0], bigram_la_model[1]);
                else
                    d.m_la = new LargeBigramLookahead(d, bigram_la_model[0]);
            }
            else {
                cerr << "unknown lookahead type: " << la_type << endl;
                exit(1);
            }
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
