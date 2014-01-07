#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "NowayHmmReader.hh"
#include "DecoderGraph.hh"

using namespace std;


void
DecoderGraph::read_phone_model(string phnfname)
{
    ifstream phnf(phnfname);
    if (!phnf) throw string("Problem opening phone model.");

    NowayHmmReader::read(phnf, m_hmms, m_hmm_map, m_num_models);
}


void
DecoderGraph::read_duration_model(string durfname)
{
    ifstream durf(durfname);
    if (!durf) throw string("Problem opening duration model.");

    NowayHmmReader::read_durations(durf, m_hmms);
}


void
DecoderGraph::read_noway_lexicon(string lexfname)
{
    ifstream lexf(lexfname);
    if (!lexf) throw string("Problem opening noway lexicon file.");

    string line;
    int linei = 1;
    while (getline(lexf, line)) {
        string unit;
        double prob = 1.0;
        vector<string> phones;

        string phone;
        stringstream ss(line);
        ss >> unit;
        while (ss >> phone) phones.push_back(phone);

        auto leftp = unit.find("(");
        if (leftp != string::npos) {
            auto rightp = unit.find(")");
            if (rightp == string::npos) throw string("Problem reading line " + linei);
            prob = stod(unit.substr(leftp+1, rightp-leftp-1));
            unit = unit.substr(0, leftp);
        }

        for (auto pit = phones.begin(); pit != phones.end(); ++pit) {
            if (m_hmm_map.find(*pit) == m_hmm_map.end())
                throw "Unknown phone " + *pit;
        }

        m_units.push_back(unit);
        m_unit_map[unit] = m_units.size()-1;

        linei++;
    }
}


void
DecoderGraph::read_word_segmentations(string segfname)
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
        if (m_word_segs.find(word) != m_word_segs.end()) throw "Error, segmentation already defined";
        string concatenated;
        while (ss >> subword) {
            if (m_unit_map.find(subword) == m_unit_map.end())
                throw "Subword " + subword + " not found in lexicon";
            m_word_segs[word].push_back(subword);
            concatenated += subword;
        }
        if (concatenated != word) throw "Erroneous segmentation: " + concatenated;

        linei++;
    }
}


void
DecoderGraph::create_word_graph(vector<SubwordNode> &nodes)
{
    nodes.resize(2);
    nodes[0].subword_id = -1; nodes[1].subword_id = -1;

    for (auto wit = m_word_segs.begin(); wit != m_word_segs.end(); ++wit) {

        int curr_nd = 0;

        for (int swidx = 0; swidx < wit->second.size(); ++swidx) {

            string curr_subword = (wit->second)[swidx];
            int curr_subword_idx = m_unit_map[curr_subword];

            if (nodes[curr_nd].out_arcs.find(curr_subword_idx) == nodes[curr_nd].out_arcs.end()) {
                nodes.resize(nodes.size()+1);
                nodes.back().subword_id = curr_subword_idx;
                nodes[curr_nd].out_arcs[curr_subword_idx] = nodes.size()-1;
                nodes.back().in_arcs.push_back(make_pair(nodes[curr_nd].subword_id, nodes.size()-1));
                curr_nd = nodes.size()-1;
            }
            else
                curr_nd = nodes[curr_nd].out_arcs[curr_subword_idx];
        }

        // Connect to end node
        nodes[curr_nd].out_arcs[-1] = 1;
        nodes[1].in_arcs.push_back(make_pair(nodes[curr_nd].subword_id, curr_nd));
    }
}


void
DecoderGraph::tie_word_graph_suffixes(vector<SubwordNode> &nodes)
{
    return;
}


void
DecoderGraph::print_word_graph(std::vector<SubwordNode> &nodes,
                               std::vector<int> path,
                               int node_idx)
{
    path.push_back(nodes[node_idx].subword_id);

    for (auto ait = nodes[node_idx].out_arcs.begin(); ait != nodes[node_idx].out_arcs.end(); ++ait) {
        if (ait->second == 1) {
            for (int i=0; i<path.size(); i++) {
                if (path[i] >= 0) cout << m_units[path[i]];
                if (i+1 != path.size()) cout << " ";
            }
            cout << endl;
        }
        else {
            print_word_graph(nodes, path, ait->second);
        }
    }
}


int
DecoderGraph::add_lm_unit(string unit)
{
    auto uit = m_unit_map.find(unit);
    if (uit != m_unit_map.end())
        return (*uit).second;

    int index = m_units.size();
    m_unit_map[unit] = index;
    m_units.push_back(unit);

    return index;
}
