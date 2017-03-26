#include <sstream>

#include "SubwordGraph.hh"
#include "LWBSubwordGraph.hh"
#include "Decoder.hh"
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
            d.m_word_boundary_symbol_idx = d.m_subword_map[d.m_word_boundary_symbol];
        }
        else if (parameter == "stats") ss >> d.m_stats;
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


void convert_nodes_for_decoder(vector<DecoderGraph::Node> &nodes,
                               vector<Decoder::Node> &dnodes)
{
    dnodes.resize(nodes.size());
    for (int i=0; i<(int)nodes.size(); i++) {
        dnodes[i].hmm_state = nodes[i].hmm_state;
        dnodes[i].word_id = nodes[i].word_id;
        dnodes[i].flags = nodes[i].flags;
        dnodes[i].arcs.resize(nodes[i].arcs.size());
        int apos=0;
        for (auto ait=nodes[i].arcs.begin(); ait != nodes[i].arcs.end(); ++ait) {
            dnodes[i].arcs[apos].target_node = (int)(*ait);
            apos++;
        }
    }
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: score [OPTION...] PH LEXICON LM CFGFILE LNALIST RESLIST\n")
    ('h', "help", "", "", "display help")
    ('n', "no-word-boundary", "", "", "Subword model without a word boundary")
    ('d', "duration-model=STRING", "arg", "", "Duration model");
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

        cerr << endl;
        print_config(d, config, cerr);

        string lnalistfname = config.arguments[4];
        ifstream lnalistf(lnalistfname);
        string line;

        string reslistfname = config.arguments[5];
        ifstream reslistf(reslistfname);
        string resline;

        int total_frames = 0;
        double total_lp = 0.0;
        double total_am_lp = 0.0;
        double total_lm_lp = 0.0;
        int file_count = 0;

        DecoderGraph *dg;
        if (config["no-word-boundary"].specified) dg = new LWBSubwordGraph();
        else dg = new SubwordGraph();
        dg->read_phone_model(phfname);
        dg->read_noway_lexicon(lexfname);

        while (getline(lnalistf, line)) {
            getline(reslistf, resline);
            if (!line.length()) continue;
            cerr << endl << "scoring: " << line << endl;

            vector<string> reswordstrs;
            stringstream ress(resline);
            string tempstr;
            while (ress >> tempstr)
                reswordstrs.push_back(tempstr);

            vector<DecoderGraph::Node> nodes;
            map<int, string> node_labels;
            dg->create_forced_path(nodes, reswordstrs, node_labels);
            dg->add_hmm_self_transitions(nodes);

            convert_nodes_for_decoder(nodes, d.m_nodes);
            d.set_hmm_transition_probs();
            d.m_decode_start_node = 0;

            int curr_frames;
            double curr_time, curr_lp, curr_am_lp, curr_lm_lp;
            d.recognize_lna_file(line, cout, &curr_frames, &curr_time,
                                 &curr_lp, &curr_am_lp, &curr_lm_lp);

            cerr << "\tframes: " << curr_frames << endl << endl;
            cerr << "\tLog prob: " << curr_lp << "\tAM: " << curr_am_lp << "\tLM: " << curr_lm_lp << endl << endl;

            total_frames += curr_frames;
            total_lp += curr_lp;
            total_am_lp += curr_am_lp;
            total_lm_lp += curr_lm_lp;
            file_count++;
        }
        lnalistf.close();

        cerr << endl;
        cerr << file_count << " files scored" << endl;
        cerr << "total frame count: " << total_frames << endl;
        cerr << "total log likelihood: " << total_lp << endl;
        cerr << "total LM likelihood: " << total_lm_lp << endl;
        cerr << "total AM likelihood: " << total_am_lp << endl;

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
