#include <sstream>

#include "Segmenter.hh"
#include "gutils.hh"
#include "conf.hh"
#include "str.hh"

using namespace std;
using namespace gutils;


void
create_forced_path(DecoderGraph &dg,
                   vector<DecoderGraph::Node> &nodes,
                   string &sentstr,
                   map<int, string> &node_labels)
{
    vector<DecoderGraph::TriphoneNode> tnodes;
    vector<string> sentence = str::split(sentstr, " ", true);

    // Create initial triphone graph
    tnodes.push_back(DecoderGraph::TriphoneNode(-1, dg.m_hmm_map["__"]));
    for (int i=0; i<(int)sentence.size(); i++) {
        vector<string> triphones;
        triphonize(sentence[i], triphones);
        for (auto tit=triphones.begin(); tit != triphones.end(); ++tit)
            tnodes.push_back(DecoderGraph::TriphoneNode(-1, dg.m_hmm_map[*tit]));
        tnodes.push_back(DecoderGraph::TriphoneNode(-1, dg.m_hmm_map["__"]));
    }

    // Convert to HMM states, also with optional crossword context
    nodes.clear(); nodes.resize(1);
    int idx = 0, crossword_start = -1;
    string crossword_left, crossword_right, label;
    node_labels.clear();

    for (int t=0; t<(int)tnodes.size(); t++)
        if (tnodes[t].hmm_id != -1) {

            if (dg.m_hmms[tnodes[t].hmm_id].label.length() == 5 &&
                dg.m_hmms[tnodes[t].hmm_id].label[4] == '_')
            {
                crossword_start = idx;
                crossword_left = dg.m_hmms[tnodes[t].hmm_id].label;
            }

            idx = connect_triphone(dg, nodes, tnodes[t].hmm_id, idx, node_labels);

            if (crossword_start != -1 &&
                dg.m_hmms[tnodes[t].hmm_id].label.length() == 5 &&
                dg.m_hmms[tnodes[t].hmm_id].label[0] == '_')
            {
                idx = connect_dummy(nodes, idx);

                crossword_right = dg.m_hmms[tnodes[t].hmm_id].label;
                crossword_left[4] = crossword_right[2];
                crossword_right[0] = crossword_left[2];

                int tmp = connect_triphone(dg, nodes, crossword_left, crossword_start, node_labels);
                tmp = connect_triphone(dg, nodes, "_", tmp, node_labels);
                tmp = connect_triphone(dg, nodes, crossword_right, tmp, node_labels);

                nodes[tmp].arcs.insert(idx);
            }

        }
        else
            idx = connect_word(nodes, tnodes[t].subword_id, idx);
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


void parse_recipe_line(string recipe_line,
                       map<string, string> &recipe_vals)
{
    recipe_line = str::cleaned(recipe_line);
    vector<string> recipe_fields = str::split(recipe_line, " \t", true);
    for (auto rfit = recipe_fields.begin(); rfit != recipe_fields.end(); ++rfit) {
        vector<string> line_vals = str::split(*rfit, "=", true);
        recipe_vals[line_vals[0]] = line_vals[1];
    }
}


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: segment [OPTION...] PH RECIPE\n")
    ('h', "help", "", "", "display help")
    ('d', "duration-model=STRING", "arg", "", "Duration model")
    ('b', "global-beam=FLOAT", "arg", "100", "Global search beam, DEFAULT: 100");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 2) config.print_help(stderr, 1);

    try {

        Segmenter s;
        s.m_global_beam = config["global-beam"].get_float();

        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        s.read_phone_model(phfname);

        if (config["duration-model"].specified) {
            string durfname = config["duration-model"].get_str();
            cerr << "Reading duration model: " << durfname << endl;
            s.read_duration_model(durfname);
        }

        string recipefname = config.arguments[1];
        ifstream recipef(recipefname);

        DecoderGraph dg;
        dg.read_phone_model(phfname);

        string recipe_line;
        while (getline(recipef, recipe_line)) {
            if (!recipe_line.length()) continue;
            map<string, string> recipe_fields;
            parse_recipe_line(recipe_line, recipe_fields);

            if (recipe_fields.find("lna") == recipe_fields.end() ||
                recipe_fields.find("text") == recipe_fields.end() ||
                recipe_fields.find("alignment") == recipe_fields.end())
                    throw string("Error in recipe line " + recipe_line);

            cerr << endl << "segmenting: " << recipe_fields["lna"] << endl;

            ifstream textf(recipe_fields["text"]);
            string resline;
            getline(textf, resline);

            vector<DecoderGraph::Node> nodes;
            map<int, string> node_labels;
            create_forced_path(dg, nodes, resline, node_labels);
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

            ofstream phnf(recipe_fields["alignment"]);
            float curr_beam = config["global-beam"].get_float();
            int attempts = 0;
            while (true) {
                bool seg_found = s.segment_lna_file(recipe_fields["lna"], node_labels, phnf);
                attempts++;
                if (seg_found) {
                    s.m_global_beam = config["global-beam"].get_float();
                    break;
                }
                else if (attempts == 5) {
                    s.m_global_beam = config["global-beam"].get_float();
                    cerr << "giving up" << endl;
                    break;
                }
                curr_beam *= 2;
                cerr << "doubling beam to " << curr_beam << endl;
                s.m_global_beam = curr_beam;
            }
            phnf.close();
        }
        recipef.close();

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
