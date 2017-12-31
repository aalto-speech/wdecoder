#ifndef CLASS_NGRAM_HH
#define CLASS_NGRAM_HH

#include <map>
#include <string>
#include <vector>

#include "io.hh"
#include "Ngram.hh"

class ClassNgram {
public:
    ClassNgram(std::string carpafname,
               std::string cmempfname,
               std::vector<std::string> text_units,
               std::map<std::string, int> text_unit_map);
    ~ClassNgram() {};
    void read_class_memberships(std::string cmempfname);
    int score(int node_idx, int word, double &score) const;
    int score(int node_idx, int word, float &score) const;
    int advance(int node_idx, int word) const { float tmp; return score(node_idx, word, tmp); }
    int order() { return m_class_ngram.max_order; };

    LNNgram m_class_ngram;
    std::map<std::string, std::pair<int, float> > m_class_memberships;
    std::vector<std::pair<int, float> > m_class_membership_lookup;
    std::vector<int> m_class_intmap;
    int m_num_classes;
    int m_sentence_end_symbol_idx;
};

#endif
