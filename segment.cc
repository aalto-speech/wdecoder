#include <sstream>
#include <string>

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
                   map<int, string> &node_labels,
                   bool breaking_short_silence,
                   bool breaking_long_silence)
{
    vector<DecoderGraph::TriphoneNode> tnodes;

    // Create initial triphone graph only with crossword context
    tnodes.push_back(DecoderGraph::TriphoneNode(-1, dg.m_hmm_map["__"]));
    vector<string> triphones;
    triphonize(sentstr, triphones);
    for (auto tit=triphones.begin(); tit != triphones.end(); ++tit)
        tnodes.push_back(DecoderGraph::TriphoneNode(-1, dg.m_hmm_map[*tit]));
    tnodes.push_back(DecoderGraph::TriphoneNode(-1, dg.m_hmm_map["__"]));

    nodes.clear(); nodes.resize(1);
    node_labels.clear();
    int idx = 0;
    for (int t=0; t<(int)tnodes.size(); t++)
        idx = connect_triphone(dg, nodes, tnodes[t].hmm_id, idx, node_labels);

    if (breaking_short_silence || breaking_long_silence) {
        int nc = nodes.size();
        for (int i=0; i<nc; i++) {
            if (node_labels.find(i) == node_labels.end() || node_labels[i] != "_.0") continue;

            string left_triphone = node_labels[i-1].substr(0, 5);
            left_triphone[4] = '_';
            string right_triphone = node_labels[i+1].substr(0, 5);
            right_triphone[0] = '_';

            int left_idx = connect_triphone(dg, nodes, left_triphone, i-4, node_labels);

            if (breaking_short_silence) {
                int idx = connect_triphone(dg, nodes, "_", left_idx, node_labels);
                idx = connect_triphone(dg, nodes, right_triphone, idx, node_labels);
                nodes[idx].arcs.insert(i+4);
            }
            if (breaking_long_silence) {
                int idx = connect_triphone(dg, nodes, "__", left_idx, node_labels);
                idx = connect_triphone(dg, nodes, right_triphone, idx, node_labels);
                nodes[idx].arcs.insert(i+4);
            }
        }
    }

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


void
print_dot_digraph(vector<Decoder::Node> &nodes,
                  ostream &fstr,
                  map<int, string> node_labels)
{
    fstr << "digraph {" << endl << endl;
    fstr << "\tnode [shape=ellipse,fontsize=30,fixedsize=false,width=0.95];" << endl;
    fstr << "\tedge [fontsize=12];" << endl;
    fstr << "\trankdir=LR;" << endl << endl;

    for (unsigned int nidx = 0; nidx < nodes.size(); ++nidx) {
        Decoder::Node &nd = nodes[nidx];
        fstr << "\t" << nidx;
        if (nd.hmm_state != -1) {
            fstr << " [label=\"" << nidx << ":" << nd.hmm_state;
            if (node_labels.find(nidx) != node_labels.end())
                fstr << ", " << node_labels[nidx] << "\"]" << endl;
            else
                fstr << "\"]" << endl;
        }
        else
            fstr << " [label=\"" << nidx << ":dummy\"]" << endl;
    }

    fstr << endl;
    for (unsigned int nidx = 0; nidx < nodes.size(); ++nidx) {
        Decoder::Node &node = nodes[nidx];
        for (auto ait = node.arcs.begin(); ait != node.arcs.end(); ++ait)
            fstr << "\t" << nidx << " -> " << ait->target_node
                 << "[label=\"" << ait->log_prob << "\"];" << endl;
    }
    fstr << "}" << endl;
}



int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: segment [OPTION...] PH RECIPE\n")
    ('h', "help", "", "", "display help")
    ('l', "long-silence", "", "", "Enable breaking long silence path between words")
    ('s', "short-silence", "", "", "Enable breaking short silence path between words")
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
            create_forced_path(dg, nodes, resline, node_labels,
                               config["short-silence"].specified,
                               config["long-silence"].specified);
            gutils::add_hmm_self_transitions(nodes);

            convert_nodes_for_decoder(nodes, s.m_nodes);
            s.set_hmm_transition_probs();
            s.m_decode_start_node = 0;

            ofstream dotf("graph.dot");
            print_dot_digraph(s.m_nodes, dotf, node_labels);
            dotf.close();
            exit(0);

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
