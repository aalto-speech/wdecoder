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


int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: segment [OPTION...] PH LNALIST RESLIST PHNLIST\n")
    ('h', "help", "", "", "display help")
    ('d', "duration-model=STRING", "arg", "", "Duration model");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 4) config.print_help(stderr, 1);

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

        string lnalistfname = config.arguments[1];
        ifstream lnalistf(lnalistfname);
        string line;

        string reslistfname = config.arguments[2];
        ifstream reslistf(reslistfname);
        string resline;

        string phnlistfname = config.arguments[3];
        ifstream phnlistf(phnlistfname);
        string phnfname;

        DecoderGraph dg;
        dg.read_phone_model(phfname);

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
