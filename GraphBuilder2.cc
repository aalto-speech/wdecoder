#include "GraphBuilder2.hh"
#include "gutils.hh"

using namespace std;
using namespace gutils;


namespace graphbuilder2 {

void
read_word_segmentations(DecoderGraph &dg,
                    string segfname,
                vector<pair<string, vector<string> > > &word_segs)
{
    ifstream segf(segfname);
    if (!segf) throw string("Problem opening word segmentations.");

    string line;
    int linei = 1;
    while (getline(segf, line)) {
        string word;
        string subword;

        stringstream ss(line);
        ss >> word;
        string concatenated;
        vector<string> tmp;
        while (ss >> subword) {
            if (m_subword_map.find(subword) == m_subword_map.end())
                throw "Subword " + subword + " not found in lexicon";
            tmp.push_back(subword);
            concatenated += subword;
        }
        if (concatenated != word) throw "Erroneous segmentation: " + concatenated;
        word_segs.push_back(make_pair(word, tmp));

        linei++;
    }
}



void
create_crossword_network(vector<pair<string, vector<string> > > &word_segs,
                                       vector<DecoderGraph::Node> &nodes,
                                       map<string, int> &fanout,
                                       map<string, int> &fanin)
{
    for (auto wit = word_segs.begin(); wit != word_segs.end(); ++wit) {
        vector<string> triphones;
        for (auto swit = wit->second.begin(); swit != wit->second.end(); ++swit) {
            for (auto tit = m_lexicon[*swit].begin(); tit != m_lexicon[*swit].end(); ++tit) {
                triphones.push_back(*tit);
            }
        }
        if (triphones.size() < 2) {
            cerr << wit->first << endl;
            throw string("Warning, word " + wit->first + " with less than two phones");
        }
        string fanint = string("_-") + triphones[0][2] + string(1,'+') + triphones[1][2];
        string fanoutt = triphones[triphones.size()-2][2] + string(1,'-') + triphones[triphones.size()-1][2] + string("+_");
        fanout[fanoutt] = -1;
        fanin[fanint] = -1;
    }

    map<string, int> connected_fanin_nodes;
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        nodes.resize(nodes.size()+1);
        nodes.back().flags |= NODE_FAN_OUT_DUMMY;
        foit->second = nodes.size()-1;

        for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
            string triphone1 = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + fiit->first[2];
            string triphone2 = foit->first[2] + string(1,'-') + fiit->first[2] + string(1,'+') + fiit->first[4];
            int idx = connect_triphone(nodes, triphone1, foit->second);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end()) {
                if (debug) cerr << "not found" << endl;
                idx = connect_triphone(nodes, triphone2, idx);
                connected_fanin_nodes[triphone2] = idx-2;
                if (fiit->second == -1) {
                    nodes.resize(nodes.size()+1);
                    nodes.back().flags |= NODE_FAN_IN_DUMMY;
                    fiit->second = nodes.size()-1;
                }
                nodes[idx].arcs.insert(fiit->second);
                if (debug) cerr << "triphone1: " << triphone1 << " triphone2: " << triphone2
                                << " target node: " << fiit->second;
            }
            else {
                if (debug) cerr << "found" << endl;
                nodes[idx].arcs.insert(connected_fanin_nodes[triphone2]);
            }
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        if (cwnit->flags == 0) cwnit->flags |= NODE_CW;
}



};

