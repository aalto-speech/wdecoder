#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

#include "Ngram.hh"

using namespace std;


void _getline(ifstream &fstr, string &line, int &linei) {
    const string read_error("Problem reading ARPA file");
    if (!getline(fstr, line)) throw read_error;
    linei++;
}


void
Ngram::read_arpa(string arpafname) {
    ifstream arpafile(arpafname);
    string header_error("Invalid ARPA header");
    if (!arpafile) throw string("Problem opening ARPA file: " + arpafname);

    int linei = 0;
    string line;
    _getline(arpafile, line, linei);
    while (line.length() == 0)
        _getline(arpafile, line, linei);

    if (line.find("\\data\\") == string::npos) throw header_error;
    _getline(arpafile, line, linei);

    int curr_ngram_order = 1;
    int total_ngram_count = 0;
    while (line.length() > 0) {
        if (line.find("ngram ") == string::npos) throw header_error;
        stringstream vals(line);
        getline(vals, line, '=');
        getline(vals, line, '=');
        int count = stoi(line);
        ngram_counts_per_order[curr_ngram_order] = count;
        total_ngram_count += count;
        curr_ngram_order++;
        _getline(arpafile, line, linei);
    }

    int max_ngram_order = curr_ngram_order-1;
    int ngrams_read = 0;
    int total_ngrams_read = 0;
    curr_ngram_order = 1;
    nodes.resize(total_ngram_count+1);
    int curr_node_idx = 1;

    while (curr_ngram_order <= max_ngram_order) {
        while (line.length() == 0)
            _getline(arpafile, line, linei);
        if (line.find("-grams") == string::npos) {
            cerr << line;
            throw header_error;
        }

        _getline(arpafile, line, linei);
        while (line.length() > 0) {
            stringstream vals(line);

            vals >> nodes[curr_node_idx].prob;
            if (nodes[curr_node_idx].prob > 0.0)
                throw string("Invalid log probability");
            nodes[curr_node_idx].node_idx = curr_node_idx;

            vector<string> curr_ngram_str;
            string tmp;
            for (int i=0; i<curr_ngram_order; i++) {
                if (vals.eof()) throw string("Invalid ARPA line");
                vals >> tmp;
                curr_ngram_str.push_back(tmp);
            }
            if (curr_ngram_order == 1) {
                vocabulary.push_back(curr_ngram_str[0]);
                vocabulary_lookup[curr_ngram_str[0]] = vocabulary.size()-1;
            }

            vector<int> curr_ngram;
            for (auto sit = curr_ngram_str.begin(); sit != curr_ngram_str.end(); ++sit)
                curr_ngram.push_back(vocabulary_lookup[*sit]);

            int node_idx_traversal = root_node;
            for (int i=0; i<curr_ngram.size()-1; i++) {
                if (nodes[node_idx_traversal].next.find(curr_ngram[i]) == nodes[node_idx_traversal].next.end()) {
                    throw string("Missing lower order n-gram");
                }
                node_idx_traversal = nodes[node_idx_traversal].next[curr_ngram[i]];
            }
            if (nodes[node_idx_traversal].next.find(curr_ngram.back()) != nodes[node_idx_traversal].next.end())
                throw string("Duplicate n-gram in model");
            nodes[node_idx_traversal].next[curr_ngram.back()] = curr_node_idx;

            nodes[curr_node_idx].word = curr_ngram.back();
            if (!vals.eof())
                vals >> nodes[curr_node_idx].backoff_prob;

            int bo_traversal = root_node;
            for (int i=1; i<curr_ngram.size(); i++)
                bo_traversal = nodes[bo_traversal].next[curr_ngram[i]];
            nodes[curr_node_idx].backoff_node = bo_traversal;

            curr_node_idx++;
            ngrams_read++;
            _getline(arpafile, line, linei);
        }

        cerr << "Read n-grams for order " << curr_ngram_order << ": " << ngrams_read << endl;
        if (ngrams_read != ngram_counts_per_order[curr_ngram_order])
            throw string("Invalid number of n-grams for order: " + curr_ngram_order);
        curr_ngram_order++;
        total_ngrams_read += ngrams_read;
        ngrams_read = 0;
    }

    sentence_start_symbol_idx = vocabulary_lookup[sentence_start_symbol];
    sentence_start_node = nodes[root_node].next[sentence_start_symbol_idx];
}


int
Ngram::score(int node_idx, int word, double &score)
{
    while (true) {
        auto nit = nodes[node_idx].next.find(word);
        if (nit != nodes[node_idx].next.end()) {
            score += nodes[nit->second].prob;
            return nit->second;
        }
        else {
            score += nodes[node_idx].backoff_prob;
            node_idx = nodes[node_idx].backoff_node;
        }
    }
}


int
Ngram::score(int node_idx, int word, float &score)
{
    while (true) {
        auto nit = nodes[node_idx].next.find(word);
        if (nit != nodes[node_idx].next.end()) {
            score += nodes[nit->second].prob;
            return nit->second;
        }
        else {
            score += nodes[node_idx].backoff_prob;
            node_idx = nodes[node_idx].backoff_node;
        }
    }
}
