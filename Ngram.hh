#include <map>
#include <string>
#include <vector>


class Ngram {
public:
    class Node {
        public:
        Node() : prob(0.0), backoff_prob(0.0), backoff_node(-1) { }
        std::map<int, int> next;
        float prob;
        float backoff_prob;
        int backoff_node;
    };

    Ngram() : root_node(0),
              sentence_start_node(-1),
              sentence_start_symbol_idx(-1)
    {
        sentence_start_symbol.assign("<s>");
    };
    ~Ngram() {};
    void read_arpa(std::string arpafname);
    int score(int node_idx, int word, double &score);
    int score(int node_idx, int word, float &score);

    int root_node;
    int sentence_start_node;
    int sentence_start_symbol_idx;
    std::string sentence_start_symbol;
    std::vector<std::string> vocabulary;
    std::map<std::string, int> vocabulary_lookup;
    std::vector<Node> nodes;
    std::map<int, int> ngram_counts_per_order;
};
