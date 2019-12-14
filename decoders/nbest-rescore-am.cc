#include <sstream>
#include <string>

#include "DecoderGraph.hh"
#include "Segmenter.hh"
#include "conf.hh"
#include "str.hh"

using namespace std;


vector<int>
create_forced_path(DecoderGraph &dg,
                   vector<DecoderGraph::Node> &nodes,
                   string &sentstr,
                   map<int, string> &node_labels,
                   bool breaking_short_silence,
                   bool breaking_long_silence)
{
    vector<int> end_nodes;
    vector<string> triphones;
    vector<int> wordIndices;
    stringstream fls(sentstr);
    string wrd;

    while (fls >> wrd) {
        if (dg.m_lexicon.find(wrd) != dg.m_lexicon.end()) {
            vector<string> &wt = dg.m_lexicon.at(wrd);
            if (wt.size() == 1) {
                cerr << "error: one phone word " << wrd << endl;
                return end_nodes;
            }
            if (triphones.size())
                triphones.push_back("_");
            for (int tr = 0; tr < (int)wt.size(); tr++)
                triphones.push_back(wt[tr]);
            wordIndices.push_back(dg.m_subword_map.at(wrd));
        }
        else {
            cerr << "error: " << wrd << " was not found in the lexicon" << endl;
            return end_nodes;
        }
    }
    if (wordIndices.size() == 0) return end_nodes;

    for (int i=1; i<(int)triphones.size()-1; i++) {
        if (triphones[i] == SHORT_SIL) {
            triphones[i-1][4] = triphones[i+1][2];
            triphones[i+1][0] = triphones[i-1][2];
        }
    }

    // Create initial triphone graph only with crossword context
    vector<TriphoneNode> tnodes;
    int wordPosition = 0;
    for (auto tit=triphones.begin(); tit != triphones.end(); ++tit) {
        if (*tit == SHORT_SIL)
            tnodes.back().subword_id = wordIndices[wordPosition++];
        tnodes.push_back(TriphoneNode(-1, dg.m_hmm_map[*tit]));
    }
    tnodes.back().subword_id = wordIndices[wordPosition];

    nodes.clear();
    nodes.resize(1);
    node_labels.clear();
    int start_idx = 0;
    int initial_ss_idx = dg.connect_triphone(nodes, dg.m_hmm_map[SHORT_SIL], start_idx, node_labels);
    int initial_ls_idx = dg.connect_triphone(nodes, dg.m_hmm_map[LONG_SIL], start_idx, node_labels);
    int word_start_idx = dg.connect_dummy(nodes, initial_ss_idx);
    int idx = word_start_idx;
    nodes[initial_ls_idx].arcs.insert(word_start_idx);

    map<int, string> wordLabels;
    for (int t=0; t<(int)tnodes.size(); t++) {
        idx = dg.connect_triphone(nodes, tnodes[t].hmm_id, idx, node_labels);
        if (tnodes[t].subword_id != -1) {
            wordLabels[idx] = dg.m_subwords[tnodes[t].subword_id];
            node_labels[idx] += " " + dg.m_subwords[tnodes[t].subword_id];
        }
    }

    int word_end_idx = idx;
    int end_ss_idx = dg.connect_triphone(nodes, dg.m_hmm_map[SHORT_SIL], word_end_idx, node_labels);
    int end_ls_idx = dg.connect_triphone(nodes, dg.m_hmm_map[LONG_SIL], word_end_idx, node_labels);
    end_nodes.push_back(end_ss_idx);
    end_nodes.push_back(end_ls_idx);

    if (breaking_short_silence || breaking_long_silence) {
        int nc = nodes.size();
        for (int i=word_start_idx; i<word_end_idx; i++) {
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

    return end_nodes;
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
    config("usage: nbest-rescore-am [OPTION...] PH LEXICON NBEST RESCORED_NBEST\n")
            ('h', "help", "", "", "display help")
            ('l', "long-silence", "", "", "Enable breaking long silence path between words")
            ('s', "short-silence", "", "", "Enable breaking short silence path between words")
            ('d', "duration-model=STRING", "arg", "", "Duration model")
            ('b', "global-beam=FLOAT", "arg", "200", "Global search beam, DEFAULT: 200.0")
            ('m', "max-tokens=INT", "arg", "500", "Maximum number of active tokens, DEFAULT: 500")
            ('o', "attempt-once", "", "", "Attempt segmentation only once without increasing beams")
            ('i', "info=INT", "arg", "0", "Info level, DEFAULT: 0");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);

    try {

        Segmenter s;
        s.m_global_beam = config["global-beam"].get_float();
        s.m_token_limit = config["max-tokens"].get_int();
        int info_level = config["info"].get_int();
        bool attempt_once = config["attempt-once"].specified;

        string ph_fname = config.arguments[0];
        string lex_fname = config.arguments[1];
        string nbest_fname = config.arguments[2];
        string rescored_nbest_fname = config.arguments[3];
        if (info_level > 0) cerr << "Reading hmms: " << ph_fname << endl;
        s.read_phone_model(ph_fname);

        if (config["duration-model"].specified) {
            string durfname = config["duration-model"].get_str();
            if (info_level > 0) cerr << "Reading duration model: " << durfname << endl;
            s.read_duration_model(durfname);
        }

        DecoderGraph dg;
        dg.read_phone_model(ph_fname);
        if (info_level > 0) cerr << "Reading lexicon: " << lex_fname << endl;
        dg.read_noway_lexicon(lex_fname);

        SimpleFileInput nbest_file(nbest_fname);
        SimpleFileOutput rescored_nbest_file(rescored_nbest_fname);
        string nbest_line;
        int linei = 0;
        double lm_scale = 0.0;
        while (nbest_file.getline(nbest_line)) {

            string lna_fname;
            double original_log_prob;
            double original_am_prob;
            double original_lm_prob;
            int num_words;
            string nbest_hypo_text, nbest_hypo_text_cleaned;

            stringstream nbest_line_ss(nbest_line);
            nbest_line_ss
                >>lna_fname
                >>original_log_prob
                >>original_am_prob
                >>original_lm_prob
                >>num_words
                >>std::ws;

            if (num_words == 0) {
                cerr << "Skipping empty hypothesis: " << nbest_line << endl;
                continue;
            }

            getline(nbest_line_ss, nbest_hypo_text);
            if (nbest_line_ss.fail()) {
                cerr << "Problem parsing line: " << nbest_line << endl;
                exit(EXIT_FAILURE);
            }

            if (linei++ == 0)
                lm_scale = (original_log_prob - original_am_prob) /  original_lm_prob;

            //cerr << lna_fname << " " << original_log_prob << " " << original_am_prob
            //     << " " << original_lm_prob << " " << num_words << " " << nbest_hypo_text << endl;
            vector<DecoderGraph::Node> nodes;
            map<int, string> node_labels;
            vector<int> end_node_idxs;

            nbest_hypo_text_cleaned.assign(nbest_hypo_text);
            string se_symbol(" </s>");
            while(nbest_hypo_text_cleaned.find(se_symbol) != std::string::npos)
                nbest_hypo_text_cleaned.replace(nbest_hypo_text_cleaned.find(se_symbol), se_symbol.size(), "");

            end_node_idxs = create_forced_path(dg, nodes, nbest_hypo_text_cleaned, node_labels,
                                               config["short-silence"].specified,
                                               config["long-silence"].specified);
            if (end_node_idxs.size() == 0) {
                cerr << "Problem creating forced path for line: " << nbest_line << endl;
                exit(EXIT_FAILURE);
            }

            dg.add_hmm_self_transitions(nodes);
            convert_nodes_for_decoder(nodes, s.m_nodes);
            s.set_hmm_transition_probs();
            s.m_decode_start_node = 0;
            s.m_decode_end_nodes = end_node_idxs;

            /*
            ofstream dotf("graph.dot");
            print_dot_digraph(s.m_nodes, dotf, node_labels);
            dotf.close();
            exit(0);
            */

            int attempts = 0;
            float rescored_am_log_prob;
            while (true) {
                rescored_am_log_prob = s.segment_lna_file(lna_fname, node_labels, nullptr);
                attempts++;
                if (rescored_am_log_prob > float(TINY_FLOAT)) {
                    if (info_level > 0) cerr << "log prob: " << rescored_am_log_prob << endl;
                    break;
                } else if (attempts >= 5 || attempt_once) {
                    cerr << "giving up" << endl;
                    break;
                }
                s.m_global_beam *= 2.0;
                s.m_token_limit *= 2;
                cerr << "increasing beam to " << s.m_global_beam << endl;
                cerr << "doubling maximum number of tokens to " << s.m_token_limit << endl;
            }
            s.m_global_beam = config["global-beam"].get_float();
            s.m_token_limit = config["max-tokens"].get_int();

            if (rescored_am_log_prob > float(TINY_FLOAT)) {
                rescored_nbest_file << lna_fname
                    << " " << (rescored_am_log_prob + lm_scale * original_lm_prob)
                    << " " << rescored_am_log_prob
                    << " " << original_lm_prob
                    << " " << num_words
                    << " " << nbest_hypo_text
                    << "\n";
            } else {
                cerr << "warning, could not find segmentation for line: " << nbest_line << endl;
            }

        }
        rescored_nbest_file.close();

    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
