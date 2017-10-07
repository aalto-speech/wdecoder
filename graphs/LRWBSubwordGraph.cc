#include <cassert>

#include "LRWBSubwordGraph.hh"

using namespace std;

LRWBSubwordGraph::LRWBSubwordGraph()
{
}

LRWBSubwordGraph::LRWBSubwordGraph(const set<string> &prefix_subwords,
                                   const set<string> &stem_subwords,
                                   const set<string> &suffix_subwords,
                                   const set<string> &word_subwords,
                                   bool verbose)
{
    create_graph(prefix_subwords,
                 stem_subwords,
                 suffix_subwords,
                 word_subwords,
                 verbose);
}

void
LRWBSubwordGraph::create_graph(const set<string> &prefix_subwords,
                               const set<string> &stem_subwords,
                               const set<string> &suffix_subwords,
                               const set<string> &word_subwords,
                               bool verbose)
{
}

void
LRWBSubwordGraph::read_lexicon(string lexfname,
                               set<string> &prefix_subwords,
                               set<string> &stem_subwords,
                               set<string> &suffix_subwords,
                               set<string> &word_subwords)
{
    read_noway_lexicon(lexfname);
    prefix_subwords.clear();
    stem_subwords.clear();
    suffix_subwords.clear();
    word_subwords.clear();

    for (auto swit = m_lexicon.begin(); swit != m_lexicon.end(); ++swit) {
        if (swit->first.find("<") != string::npos) continue;
        if (swit->first.length() == 0) continue;

        if (swit->first.front() == '_' && swit->first.back() == '_')
            word_subwords.insert(swit->first);
        else if (swit->first.front() == '_')
            prefix_subwords.insert(swit->first);
        else if (swit->first.back() == '_')
            suffix_subwords.insert(swit->first);
        else
            stem_subwords.insert(swit->first);
    }
}
