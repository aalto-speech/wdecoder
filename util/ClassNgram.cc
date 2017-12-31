#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#include "ClassNgram.hh"
#include "str.hh"
#include "defs.hh"

using namespace std;


ClassNgram::ClassNgram(
    string carpafname,
    string cmempfname,
    vector<string> text_units,
    map<string, int> text_unit_map)
{
    cerr << "Reading class n-gram: " << carpafname << endl;
    m_class_ngram.read_arpa(carpafname);
    cerr << "Reading class membership probs.." << cmempfname << endl;
    read_class_memberships(cmempfname);

    m_class_membership_lookup.resize(text_units.size(), make_pair(-1,MIN_LOG_PROB));
    for (auto wpit=m_class_memberships.begin(); wpit!= m_class_memberships.end(); ++wpit) {
        if (wpit->first == "<unk>") continue;
        if (text_unit_map.find(wpit->first) == text_unit_map.end()) continue;
        int word_idx = text_unit_map.at(wpit->first);
        m_class_membership_lookup[word_idx] = wpit->second;
    }
    if (text_unit_map.find("</s>") == text_unit_map.end()) {
        cerr << "sentence end symbol not found in the text units" << endl;
        exit(1);
    }
    m_sentence_end_symbol_idx = text_unit_map.at("</s>");

    m_class_intmap.resize(m_num_classes);
    for (int i=0; i<(int)m_class_intmap.size(); i++)
        m_class_intmap[i] = m_class_ngram.vocabulary_lookup[int2str(i)];
}


void
ClassNgram::read_class_memberships(string cmempfname)
{
    SimpleFileInput wcf(cmempfname);

    string line;
    int max_class_idx = 0;
    while (wcf.getline(line)) {
        stringstream ss(line);
        string word;
        int class_idx;
        double prob;
        ss >> word >> class_idx >> prob;
        if (ss.fail()) {
            cerr << "problem parsing line: " << line << endl;
            exit(1);
        }
        m_class_memberships[word] = make_pair(class_idx, prob);
        max_class_idx = max(max_class_idx, class_idx);
    }
    m_num_classes = max_class_idx + 1;
}


int
ClassNgram::score(int node_idx, int word_id, double &score) const
{
    if (word_id >= (int)m_class_membership_lookup.size()
        || m_class_membership_lookup[word_id].second == MIN_LOG_PROB) {
        cerr << "word with id " << word_id << " not initialized" << endl;
        exit(1);
    }

    double membership_prob = m_class_membership_lookup[word_id].second;
    double ngram_prob = 0.0;
    int new_node_idx;
    if (word_id == m_sentence_end_symbol_idx) {
        new_node_idx = m_class_ngram.score(
            node_idx,
            m_class_ngram.sentence_end_symbol_idx,
            ngram_prob);
        score = ngram_prob;
    }
    else {
        new_node_idx = m_class_ngram.score(
            node_idx,
            m_class_intmap[m_class_membership_lookup[word_id].first],
            ngram_prob);
        score = membership_prob + ngram_prob;
    }

    return new_node_idx;
}


int
ClassNgram::score(int node_idx, int word_id, float &score) const
{
    if (word_id >= (int)m_class_membership_lookup.size()
        || m_class_membership_lookup[word_id].second == MIN_LOG_PROB) {
        cerr << "word with id " << word_id << " not initialized" << endl;
        exit(1);
    }

    double membership_prob = m_class_membership_lookup[word_id].second;
    double ngram_prob = 0.0;
    int new_node_idx;
    if (word_id == m_sentence_end_symbol_idx) {
        new_node_idx = m_class_ngram.score(
            node_idx,
            m_class_ngram.sentence_end_symbol_idx,
            ngram_prob);
        score = ngram_prob;
    }
    else {
        new_node_idx = m_class_ngram.score(
            node_idx,
            m_class_intmap[m_class_membership_lookup[word_id].first],
            ngram_prob);
        score = membership_prob + ngram_prob;
    }

    return new_node_idx;
}

