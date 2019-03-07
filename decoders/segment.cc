#include <sstream>
#include <string>

#include "DecoderGraph.hh"
#include "Segmenter.hh"
#include "conf.hh"
#include "str.hh"

using namespace std;


int
create_forced_path(DecoderGraph &dg,
                   vector<DecoderGraph::Node> &nodes,
                   string &sentstr,
                   map<int, string> &node_labels,
                   bool breaking_short_silence,
                   bool breaking_long_silence)
{
    vector<string> triphones;
    vector<int> wordIndices;
    stringstream fls(sentstr);
    string wrd;

    while (fls >> wrd) {
        if (dg.m_lexicon.find(wrd) != dg.m_lexicon.end()) {
            vector<string> &wt = dg.m_lexicon.at(wrd);
            if (wt.size() == 1) {
                cerr << "error: one phone word " << wrd << endl;
                return -1;
            }
            if (triphones.size())
                triphones.push_back("_");
            for (int tr = 0; tr < (int)wt.size(); tr++)
                triphones.push_back(wt[tr]);
            wordIndices.push_back(dg.m_subword_map.at(wrd));
        }
        else {
            cerr << "error: " << wrd << " was not found in the lexicon" << endl;
            return -1;
        }
    }
    if (wordIndices.size() == 0) return -1;

    for (int i=1; i<(int)triphones.size()-1; i++) {
        if (triphones[i] == SHORT_SIL) {
            triphones[i-1][4] = triphones[i+1][2];
            triphones[i+1][0] = triphones[i-1][2];
        }
    }

    // Create initial triphone graph only with crossword context
    vector<TriphoneNode> tnodes;
    int wordPosition = 0;
    tnodes.push_back(TriphoneNode(-1, dg.m_hmm_map[LONG_SIL]));
    for (auto tit=triphones.begin(); tit != triphones.end(); ++tit) {
        if (*tit == SHORT_SIL)
            tnodes.back().subword_id = wordIndices[wordPosition++];
        tnodes.push_back(TriphoneNode(-1, dg.m_hmm_map[*tit]));
    }
    tnodes.back().subword_id = wordIndices[wordPosition];
    tnodes.push_back(TriphoneNode(-1, dg.m_hmm_map[LONG_SIL]));

    nodes.clear(); nodes.resize(1);
    node_labels.clear();
    int idx = 0;
    map<int, string> wordLabels;
    for (int t=0; t<(int)tnodes.size(); t++) {
        idx = dg.connect_triphone(nodes, tnodes[t].hmm_id, idx, node_labels);
        if (tnodes[t].subword_id != -1) {
            wordLabels[idx] = dg.m_subwords[tnodes[t].subword_id];
            node_labels[idx] += " " + dg.m_subwords[tnodes[t].subword_id];
        }
    }
    int end_idx = idx;

    if (breaking_short_silence || breaking_long_silence) {
        int nc = nodes.size();
        for (int i=0; i<nc; i++) {
            if (node_labels.find(i) == node_labels.end()
                || node_labels[i] != "_.0") continue;

            string left_triphone = node_labels[i-1].substr(0, 5);
            left_triphone[4] = SIL_CTXT;
            string right_triphone = node_labels[i+1].substr(0, 5);
            right_triphone[0] = SIL_CTXT;

            int left_idx = dg.connect_triphone(nodes, left_triphone, i-4, node_labels);
            if (wordLabels.find(i-1) != wordLabels.end())
                node_labels[left_idx] += " " + wordLabels[i-1];

            if (breaking_short_silence) {
                int idx = dg.connect_triphone(nodes, SHORT_SIL, left_idx, node_labels);
                idx = dg.connect_triphone(nodes, right_triphone, idx, node_labels);
                nodes[idx].arcs.insert(i+4);
            }
            if (breaking_long_silence) {
                int idx = dg.connect_triphone(nodes, LONG_SIL, left_idx, node_labels);
                idx = dg.connect_triphone(nodes, right_triphone, idx, node_labels);
                nodes[idx].arcs.insert(i+4);
            }
        }
    }

    return end_idx;
}


int
parse_transcript_phn(DecoderGraph &dg,
                     vector<DecoderGraph::Node> &nodes,
                     string phnfname,
                     map<int, string> &node_labels)
{
    ifstream phnf(phnfname);

    nodes.clear(); nodes.resize(1);
    node_labels.clear();
    int idx = 0;

    string phn_line;
    while (getline(phnf, phn_line)) {
        phn_line = str::cleaned(phn_line);
        vector<string> parts = str::split(phn_line, " \t", true);
        if (parts.size() != 3) throw string("Invalid phn line: ") + phn_line;
        if (parts[2].find(".0") == string::npos) continue;
        string phone = parts[2].substr(0, parts[2].size()-2);
        idx = dg.connect_triphone(nodes, phone, idx, node_labels);
    }

    return idx;
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
        for (auto ait=nodes[i].arcs.begin(); ait != nodes[i].arcs.end(); ++ait)
            dnodes[i].arcs[apos++].target_node = (int)(*ait);
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
get_recipe_lines(string recipe_fname,
                 int num_batches,
                 int batch_index,
                 vector<string> &recipe_lines)
{
    ifstream recipef(recipe_fname);
    string recipe_line;

    int line_count = 0;
    while (getline(recipef, recipe_line)) {
        recipe_line = str::cleaned(recipe_line);
        if (!recipe_line.length()) continue;
        line_count++;
    }

    int line_idx = 0;
    int start_idx = 0;
    int end_idx = line_count;
    if (num_batches > 0 && batch_index > 0) {
        start_idx = (batch_index-1) * (line_count / num_batches);
        end_idx = (batch_index) * (line_count / num_batches);
        if (num_batches == batch_index) end_idx = line_count;

        int remainder = line_count % num_batches;
        start_idx += min(batch_index-1, remainder);
        end_idx += min(batch_index, remainder);
    }

    ifstream recipef2(recipe_fname);
    while (getline(recipef2, recipe_line)) {
        recipe_line = str::cleaned(recipe_line);
        if (!recipe_line.length()) continue;
        if (line_idx >= start_idx && line_idx < end_idx)
            recipe_lines.push_back(recipe_line);
        line_idx++;
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
    ('t', "text-field", "", "", "Create alignment from text field, -x must be defined as well")
    ('x', "lexicon=STRING", "arg", "", "Lexicon file to be used with -t")
    ('l', "long-silence", "", "", "Enable breaking long silence path between words")
    ('s', "short-silence", "", "", "Enable breaking short silence path between words")
    ('d', "duration-model=STRING", "arg", "", "Duration model")
    ('b', "global-beam=FLOAT", "arg", "200", "Global search beam, DEFAULT: 200.0")
    ('m', "max-tokens=INT", "arg", "500", "Maximum number of active tokens, DEFAULT: 500")
    ('n', "lna-dir=STRING", "arg", "", "LNA directory")
    ('o', "attempt-once", "", "", "Attempt segmentation only once without increasing beams")
    ('B', "batch=INT", "arg", "0", "number of batch processes with the same recipe")
    ('I', "bindex=INT", "arg", "0", "batch process index")
    ('i', "info=INT", "arg", "0", "Info level, DEFAULT: 0");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 2) config.print_help(stderr, 1);

    try {

        Segmenter s;
        s.m_global_beam = config["global-beam"].get_float();
        s.m_token_limit = config["max-tokens"].get_int();
        int info_level = config["info"].get_int();
        bool attempt_once = config["attempt-once"].specified;

        if (!config["text-field"].specified &&
            (config["long-silence"].specified || config["short-silence"].specified))
               throw string("Silence options are only usable with text-field switch");

        if (config["text-field"].specified && !config["lexicon"].specified)
            throw string("Lexicon needs to be set with -t option");

        string phfname = config.arguments[0];
        if (info_level > 0) cerr << "Reading hmms: " << phfname << endl;
        s.read_phone_model(phfname);

        if (config["duration-model"].specified) {
            string durfname = config["duration-model"].get_str();
            if (info_level > 0) cerr << "Reading duration model: " << durfname << endl;
            s.read_duration_model(durfname);
        }

        string recipefname = config.arguments[1];
        vector<string> recipe_lines;
        get_recipe_lines(recipefname, config["batch"].get_int(), config["bindex"].get_int(), recipe_lines);

        ifstream recipef(recipefname);

        DecoderGraph dg;
        dg.read_phone_model(phfname);

        if (config["lexicon"].specified) {
            string lexfname = config["lexicon"].get_str();
            if (info_level > 0) cerr << "Reading lexicon: " << lexfname << endl;
            dg.read_noway_lexicon(lexfname);
        }

        for (auto rlit = recipe_lines.begin(); rlit != recipe_lines.end(); ++rlit) {

            map<string, string> recipe_fields;
            parse_recipe_line(*rlit, recipe_fields);

            if (recipe_fields.find("lna") == recipe_fields.end() ||
                recipe_fields.find("alignment") == recipe_fields.end())
                    throw string("Error in recipe line " + *rlit);

            if (config["text-field"].specified) {
                if (recipe_fields.find("text") == recipe_fields.end())
                    throw string("Error in recipe line " + *rlit);
            }
            else {
                if (recipe_fields.find("transcript") == recipe_fields.end())
                    throw string("Error in recipe line " + *rlit);
            }

            string lna_file = recipe_fields["lna"];
            if (config["lna-dir"].specified) lna_file = config["lna-dir"].get_str() + "/" + lna_file;

            if (info_level > 0) cerr << "segmenting to: " << recipe_fields["alignment"] << endl;

            vector<DecoderGraph::Node> nodes;
            map<int, string> node_labels;
            int end_node_idx;

            if (config["text-field"].specified) {
                ifstream textf(recipe_fields["text"]);
                string resline;
                string fileline;
                while (getline(textf, fileline)) {
                    resline += fileline;
                    resline += "\n";
                }
                end_node_idx = create_forced_path(dg, nodes, resline, node_labels,
                                                  config["short-silence"].specified,
                                                  config["long-silence"].specified);
                if (end_node_idx == -1) continue;
            }
            else {
                end_node_idx = parse_transcript_phn(dg, nodes,
                                                    recipe_fields["transcript"],
                                                    node_labels);
            }

            dg.add_hmm_self_transitions(nodes);
            convert_nodes_for_decoder(nodes, s.m_nodes);
            s.set_hmm_transition_probs();
            s.m_decode_start_node = 0;
            s.m_decode_end_node = end_node_idx;

            /*
            ofstream dotf("graph.dot");
            print_dot_digraph(s.m_nodes, dotf, node_labels);
            dotf.close();
            exit(0);
            */

            ofstream phnf(recipe_fields["alignment"]);
            int attempts = 0;
            while (true) {
                float log_prob = s.segment_lna_file(lna_file, node_labels, phnf);
                attempts++;
                if (log_prob > float(TINY_FLOAT)) {
                    if (info_level > 0) cerr << "log prob: " << log_prob << endl;
                    break;
                } else if (attempts >= 3 || attempt_once) {
                    cerr << "giving up" << endl;
                    break;
                }
                s.m_global_beam *= 1.5;
                s.m_token_limit *= 2;
                cerr << "increasing beam to " << s.m_global_beam << endl;
                cerr << "doubling maximum number of tokens to " << s.m_token_limit << endl;
            }
            s.m_global_beam = config["global-beam"].get_float();
            s.m_token_limit = config["max-tokens"].get_int();
            phnf.close();
        }
        recipef.close();

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
