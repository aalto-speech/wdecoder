#include <sstream>

#include "SubwordGraphBuilder.hh"
#include "Segmenter.hh"
#include "gutils.hh"
#include "conf.hh"

using namespace std;


void
read_config(Segmenter &s, string cfgfname)
{
    ifstream cfgf(cfgfname);
    if (!cfgf) throw string("Problem opening configuration file: ") + cfgfname;

    string line;
    while (getline(cfgf, line)) {
        if (!line.length()) continue;
        stringstream ss(line);
        string parameter, val;
        ss >> parameter;
        if (parameter == "lm_scale") ss >> s.m_lm_scale;
        else if (parameter == "token_limit") ss >> s.m_token_limit;
        else if (parameter == "node_limit") ss >> s.m_active_node_limit;
        else if (parameter == "duration_scale") ss >> s.m_duration_scale;
        else if (parameter == "transition_scale") ss >> s.m_transition_scale;
        else if (parameter == "global_beam") ss >> s.m_global_beam;
        else if (parameter == "acoustic_beam") continue;
        else if (parameter == "history_beam") continue;
        else if (parameter == "word_end_beam") ss >> s.m_word_end_beam;
        else if (parameter == "node_beam") ss >> s.m_node_beam;
        else if (parameter == "word_boundary_penalty") ss >> s.m_word_boundary_penalty;
        else if (parameter == "history_clean_frame_interval") ss >> s.m_history_clean_frame_interval;
        else if (parameter == "force_sentence_end") {
            string force_str;
            ss >> force_str;
            s.m_force_sentence_end = true ? force_str == "true": false;
        }
        else if (parameter == "word_boundary_symbol") {
            s.m_use_word_boundary_symbol = true;
            ss >> s.m_word_boundary_symbol;
            s.m_word_boundary_symbol_idx = s.m_subword_map[s.m_word_boundary_symbol];
        }
        else if (parameter == "debug") ss >> s.m_debug;
        else if (parameter == "stats") ss >> s.m_stats;
        else if (parameter == "token_stats") ss >> s.m_token_stats;
        else throw string("Unknown parameter: ") + parameter;
    }

    cfgf.close();
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
    config("usage: segment [OPTION...] PH LEXICON LNALIST RESLIST PHNLIST\n")
    ('h', "help", "", "", "display help")
    ('d', "duration-model=STRING", "arg", "", "Duration model");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 5) config.print_help(stderr, 1);

    try {

        Segmenter s;

        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        s.read_phone_model(phfname);

        if (config["duration-model"].specified) {
            string durfname = config["duration-model"].get_str();
            cerr << "Reading duration model: " << durfname << endl;
            s.read_duration_model(durfname);
        }

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        s.read_noway_lexicon(lexfname);

        string lnalistfname = config.arguments[2];
        ifstream lnalistf(lnalistfname);
        string line;

        string reslistfname = config.arguments[3];
        ifstream reslistf(reslistfname);
        string resline;

        string phnlistfname = config.arguments[4];
        ifstream phnlistf(phnlistfname);
        string phnfname;

        DecoderGraph dg;
        dg.read_phone_model(phfname);
        dg.read_noway_lexicon(lexfname);

        while (getline(lnalistf, line)) {
            getline(reslistf, resline);
            getline(phnlistf, phnfname);
            if (!line.length()) continue;
            cerr << endl << "segmenting: " << line << endl;

            vector<string> reswordstrs;
            stringstream ress(resline);
            string tempstr;
            while (ress >> tempstr)
                reswordstrs.push_back(tempstr);

            vector<DecoderGraph::Node> nodes;
            map<int, string> node_labels;
            subwordgraphbuilder::create_forced_path(dg, nodes, reswordstrs, node_labels);
            gutils::add_hmm_self_transitions(nodes);

            convert_nodes_for_decoder(nodes, s.m_nodes);
            s.set_hmm_transition_probs();
            s.m_decode_start_node = 0;

            /*
            ofstream dotf("graph.dot");
            s.print_dot_digraph(s.m_nodes, dotf);
            dotf.close();
            exit(0);
            */

            ofstream phnf(phnfname);
            s.segment_lna_file(line, node_labels, phnf);
            phnf.close();
        }
        lnalistf.close();

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
