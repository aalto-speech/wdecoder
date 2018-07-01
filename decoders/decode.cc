#include <sstream>
#include <thread>

#include "NgramDecoder.hh"
#include "Lookahead.hh"
#include "ClassLookahead.hh"
#include "conf.hh"
#include "str.hh"
#include "decoder-helpers.hh"

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
    ('n', "nbest=STRING", "arg", "", "N-best list file (use .gz suffix for compression)")
    ('y', "nbest-num-hypotheses", "arg", "10000", "Maximum number of hypotheses per file")
    ('b', "nbest-beam", "arg", "1000.0", "Beam setting (total difference from the best hypothesis log prob)");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 6) config.print_help(stderr, 1);

    try {
        NgramDecoder d;

        string phfname = config.arguments[0];
        string lexfname = config.arguments[1];
        string lmfname = config.arguments[2];
        string cfgfname = config.arguments[3];
        string graphfname = config.arguments[4];
        string lnalistfname = config.arguments[5];

        cerr << "Reading hmms: " << phfname << endl;
        d.read_phone_model(phfname);

        if (config["duration-model"].specified) {
            string durfname = config["duration-model"].get_str();
            cerr << "Reading duration model: " << durfname << endl;
            d.read_duration_model(durfname);
        }

        cerr << "Reading lexicon: " << lexfname << endl;
        d.read_noway_lexicon(lexfname);

        // Read before loading language model
        // for possible word boundary symbol configuration
        cerr << "Reading configuration: " << cfgfname << endl;
        read_config(d, cfgfname);

        cerr << "Reading language model: " << lmfname << endl;
        d.read_lm(lmfname);

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

        SimpleFileOutput *nbest = config["nbest"].specified ? new SimpleFileOutput(config["nbest"].get_str()) : nullptr;
        if (config["result-file"].specified) {
            string resultfname = config["result-file"].get_str();
            cerr << "Base filename for results: " << resultfname << endl;
            string resfname = resultfname + string(".rec");
            string logfname = resultfname + string(".log");
            ofstream resultf(resfname);
            ofstream logf(logfname);
            recognize_lnas(d, config, lnalistfname, resultf, logf, nbest);
        }
        else recognize_lnas(d, config, lnalistfname, cout, cerr, nbest);
        if (nbest != nullptr) {
            nbest->close();
            delete nbest;
        }

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
