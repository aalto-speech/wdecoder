#include <sstream>
#include <string>
#include <thread>

#include "DecoderGraph.hh"
#include "Segmenter.hh"
#include "conf.hh"
#include "str.hh"

using namespace std;


struct NbestFileEntry {
public:
    NbestFileEntry() {
        rescored = false;
        rescore_success = true;
    };

    void print_rescored_entry(SimpleFileOutput &output, double lm_scale) {
        output << lna_fname
               << " " << (rescored_am_prob + lm_scale * original_lm_prob)
               << " " << rescored_am_prob
               << " " << original_lm_prob
               << " " << num_words
               << " " << nbest_hypo_text
               << "\n";
    }

    string lna_fname;
    double original_log_prob;
    double original_am_prob;
    double original_lm_prob;
    double rescored_am_prob;
    int num_words;
    string original_line;
    string nbest_hypo_text;
    string nbest_hypo_text_cleaned;
    bool rescored;
    bool rescore_success;
};


vector<int>
create_forced_path(const DecoderGraph &dg,
                   vector<DecoderGraph::Node> &nodes,
                   string &sentstr,
                   map<int, string> &node_labels,
                   bool breaking_short_silence,
                   bool breaking_long_silence,
                   bool subwords_with_word_boundary)
{
    vector<int> end_nodes;
    vector<string> triphones;
    vector<int> wordIndices;
    stringstream fls(sentstr);

    if (subwords_with_word_boundary) {
        string subword;
        vector<string> curr_word_triphones;
        while (fls >> subword) {
            if (dg.m_lexicon.find(subword) == dg.m_lexicon.end()) {
                cerr << "error: " << subword << " was not found in the lexicon" << endl;
                return end_nodes;
            }

            if (subword == "<w>") {
                if (curr_word_triphones.size() == 1) {
                    cerr << "error: one phone word " << curr_word_triphones[0] << endl;
                    return end_nodes;
                }

                if (curr_word_triphones.size() > 0) {
                    for (int i = 1; i < (int)curr_word_triphones.size(); i++)
                        if (curr_word_triphones[i][0] == '_')
                            curr_word_triphones[i][0] = curr_word_triphones[i-1][2];
                    for (int i = 0; i < (int)curr_word_triphones.size()-1; i++)
                        if (curr_word_triphones[i][4] == '_')
                            curr_word_triphones[i][4] = curr_word_triphones[i+1][2];

                    if (triphones.size()) triphones.push_back(SHORT_SIL);
                    for (int i = 0; i < (int)curr_word_triphones.size(); i++)
                        triphones.push_back(curr_word_triphones[i]);
                    curr_word_triphones.clear();
                }
            } else {
                const vector<string> &swt = dg.m_lexicon.at(subword);
                for (int tr = 0; tr < (int)swt.size(); tr++)
                    curr_word_triphones.push_back(swt[tr]);
            }
            wordIndices.push_back(dg.m_subword_map.at(subword));
        }
    } else {
        string wrd;
        while (fls >> wrd) {
            if (dg.m_lexicon.find(wrd) == dg.m_lexicon.end()) {
                cerr << "error: " << wrd << " was not found in the lexicon" << endl;
                return end_nodes;
            }

            const vector <string> &wt = dg.m_lexicon.at(wrd);
            if (wt.size() == 1) {
                cerr << "error: one phone word " << wrd << endl;
                return end_nodes;
            }
            if (triphones.size())
                triphones.push_back(SHORT_SIL);
            for (int tr = 0; tr < (int) wt.size(); tr++)
                triphones.push_back(wt[tr]);
            wordIndices.push_back(dg.m_subword_map.at(wrd));
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
        tnodes.push_back(TriphoneNode(-1, dg.m_hmm_map.at(*tit)));
    }
    tnodes.back().subword_id = wordIndices[wordPosition];

    nodes.clear();
    nodes.resize(1);
    node_labels.clear();
    int start_idx = 0;
    int initial_ss_idx = dg.connect_triphone(nodes, dg.m_hmm_map.at(SHORT_SIL), start_idx, node_labels);
    int initial_ls_idx = dg.connect_triphone(nodes, dg.m_hmm_map.at(LONG_SIL), start_idx, node_labels);
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
    int end_ss_idx = dg.connect_triphone(nodes, dg.m_hmm_map.at(SHORT_SIL), word_end_idx, node_labels);
    int end_ls_idx = dg.connect_triphone(nodes, dg.m_hmm_map.at(LONG_SIL), word_end_idx, node_labels);
    end_nodes.push_back(end_ss_idx);
    end_nodes.push_back(end_ls_idx);

    if (breaking_short_silence || breaking_long_silence) {
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


void
rescore_nbest_entry(Segmenter &s,
                    const DecoderGraph &dg,
                    const conf::Config &config,
                    NbestFileEntry &curr_entry)
{
    int info_level = config["info"].get_int();
    bool attempt_once = config["attempt-once"].specified;

    vector<DecoderGraph::Node> nodes;
    map<int, string> node_labels;
    vector <int> end_node_idxs = create_forced_path(
                                       dg, nodes, curr_entry.nbest_hypo_text_cleaned, node_labels,
                                       config["short-silence"].specified,
                                       config["long-silence"].specified,
                                       config["subwords-with-word-boundary"].specified);
    if (end_node_idxs.size() == 0) {
        cerr << "Problem creating forced path for line: " << curr_entry.original_line << endl;
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
        rescored_am_log_prob = s.segment_lna_file(curr_entry.lna_fname, node_labels, nullptr, 0);
        attempts++;
        if (rescored_am_log_prob > float(TINY_FLOAT)) {
            if (info_level > 0) cerr << "log prob: " << rescored_am_log_prob << endl;
            break;
        } else if (attempts >= 5 || attempt_once) {
            break;
        }
        s.m_global_beam *= 2.0;
        s.m_token_limit *= 2;
        if (info_level > 1)
            cerr << "trying beam " << s.m_global_beam << " and maximum tokens " << s.m_token_limit << endl;
    }
    s.m_global_beam = config["global-beam"].get_float();
    s.m_token_limit = config["max-tokens"].get_int();

    if (rescored_am_log_prob > float(TINY_FLOAT)) {
        curr_entry.rescored_am_prob = rescored_am_log_prob;
        curr_entry.rescored = true;
        curr_entry.rescore_success = true;
    } else {
        if (info_level > 0)
            cerr << "warning, could not find segmentation for line: " << curr_entry.original_line << endl;
        curr_entry.rescored = true;
        curr_entry.rescore_success = false;
    }
}


void
start_rescore_worker(
    Segmenter &s,
    vector<NbestFileEntry> &nbest_entries,
    const DecoderGraph &dg,
    const conf::Config &config,
    int thread_idx,
    int num_threads)
{
    for (int i=0; i<(int)nbest_entries.size(); i++) {
        if (i % num_threads == thread_idx) {
            cerr << i + 1 << "/" << nbest_entries.size() << " ";
            rescore_nbest_entry(s, dg, config, nbest_entries[i]);
        }
    }
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
            ('t', "num-threads", "arg", "1", "Number of threads")
            ('w', "subwords-with-word-boundary", "", "",
                    "Subword lexicon with the word boundary symbol <w>, otherwise a word lexicon is assumed")
            ('i', "info=INT", "arg", "0", "Info level, DEFAULT: 0");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);

    try {
        string ph_fname = config.arguments[0];
        string lex_fname = config.arguments[1];
        string nbest_fname = config.arguments[2];
        string rescored_nbest_fname = config.arguments[3];
        int info_level = config["info"].get_int();
        int num_threads = config["num-threads"].get_int();

        DecoderGraph dg;
        dg.read_phone_model(ph_fname);
        if (info_level > 0) cerr << "Reading lexicon: " << lex_fname << endl;
        dg.read_noway_lexicon(lex_fname);

        vector<Segmenter> segmenters(num_threads);
        for (int t=0; t<num_threads; t++) {
            if (info_level > 0) cerr << "Initializing segmenter " << t << endl;
            segmenters[t].m_global_beam = config["global-beam"].get_float();
            segmenters[t].m_token_limit = config["max-tokens"].get_int();
            segmenters[t].read_phone_model(ph_fname);
            if (config["duration-model"].specified)
                segmenters[t].read_duration_model(config["duration-model"].get_str());
        }

        SimpleFileInput nbest_file(nbest_fname);
        string nbest_line;
        double lm_scale = 0.0;
        vector<NbestFileEntry> nbest_entries;
        while (nbest_file.getline(nbest_line)) {
            NbestFileEntry curr_entry;
            curr_entry.original_line.assign(nbest_line);
            stringstream nbest_line_ss(nbest_line);
            nbest_line_ss
                    >> curr_entry.lna_fname
                    >> curr_entry.original_log_prob
                    >> curr_entry.original_am_prob
                    >> curr_entry.original_lm_prob
                    >> curr_entry.num_words
                    >> std::ws;

            if (curr_entry.num_words == 0) {
                cerr << "Skipping empty hypothesis: " << nbest_line << endl;
                continue;
            }

            getline(nbest_line_ss, curr_entry.nbest_hypo_text);
            if (nbest_line_ss.fail()) {
                cerr << "Problem parsing line: " << nbest_line << endl;
                exit(EXIT_FAILURE);
            }

            curr_entry.nbest_hypo_text_cleaned.assign(curr_entry.nbest_hypo_text);
            string se_symbol(" </s>");
            while(curr_entry.nbest_hypo_text_cleaned.find(se_symbol) != std::string::npos)
                curr_entry.nbest_hypo_text_cleaned.replace(curr_entry.nbest_hypo_text_cleaned.find(se_symbol),
                                                           se_symbol.size(), "");

            if (nbest_entries.size() == 0)
                lm_scale = (curr_entry.original_log_prob - curr_entry.original_am_prob)
                           / curr_entry.original_lm_prob;
            nbest_entries.push_back(curr_entry);
        }

        vector<std::thread*> threads;
        for (int t=0; t<num_threads; t++) {
            thread *thr = new std::thread(&start_rescore_worker,
                                          std::ref(segmenters[t]),
                                          std::ref(nbest_entries),
                                          std::cref(dg),
                                          std::cref(config),
                                          t,
                                          num_threads);
            threads.push_back(thr);
        }

        int curr_line_idx = 0;
        SimpleFileOutput rescored_nbest_file(rescored_nbest_fname);
        while (curr_line_idx < (int)nbest_entries.size()) {
            if (nbest_entries[curr_line_idx].rescored) {
                if (nbest_entries[curr_line_idx].rescore_success)
                    nbest_entries[curr_line_idx].print_rescored_entry(rescored_nbest_file, lm_scale);
                curr_line_idx++;
            }
            else sleep(2);
        }
        rescored_nbest_file.close();

        for (int t=0; t<num_threads; t++)
            threads[t]->join();


    } catch (string &e) {
        cerr << e << endl;
    }

    exit(EXIT_SUCCESS);
}
