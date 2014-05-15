#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "NowayHmmReader.hh"
#include "DecoderGraph.hh"
#include "WordGraphBuilder.hh"
#include "gutils.hh"

using namespace std;
using namespace gutils;


namespace wordgraphbuilder {


void
create_crossword_network(DecoderGraph &dg,
                         set<string> &words,
                         vector<DecoderGraph::Node> &nodes,
                         map<string, int> &fanout,
                         map<string, int> &fanin)
{
    for (auto wit = words.begin(); wit != words.end(); ++wit) {

        vector<string> &triphones = dg.m_lexicon[*wit];
        if (triphones.size() == 0) continue;
        else if (triphones.size() == 1) {
            continue;
        }
        else {
            string fanint = string("_-") + triphones[0][2] + string(1,'+') + triphones[1][2];
            string fanoutt = triphones[triphones.size()-2][2] + string(1,'-') + triphones[triphones.size()-1][2] + string("+_");
            fanout[fanoutt] = -1;
            fanin[fanint] = -1;
        }
    }

    map<string, int> connected_fanin_nodes;
    for (auto foit = fanout.begin(); foit != fanout.end(); ++foit) {

        nodes.resize(nodes.size()+1);
        nodes.back().flags |= NODE_FAN_OUT_DUMMY;
        foit->second = nodes.size()-1;

        for (auto fiit = fanin.begin(); fiit != fanin.end(); ++fiit) {
            string triphone1 = foit->first[0] + string(1,'-') + foit->first[2] + string(1,'+') + fiit->first[2];
            string triphone2 = foit->first[2] + string(1,'-') + fiit->first[2] + string(1,'+') + fiit->first[4];

            int idx = connect_triphone(dg, nodes, triphone1, foit->second);
            idx = connect_triphone(dg, nodes, "_", idx);

            if (connected_fanin_nodes.find(triphone2) == connected_fanin_nodes.end()) {
                idx = connect_triphone(dg, nodes, triphone2, idx);
                connected_fanin_nodes[triphone2] = idx-2;
                if (fiit->second == -1) {
                    nodes.resize(nodes.size()+1);
                    nodes.back().flags |= NODE_FAN_IN_DUMMY;
                    fiit->second = nodes.size()-1;
                }
                nodes[idx].arcs.insert(fiit->second);
            }
            else
                nodes[idx].arcs.insert(connected_fanin_nodes[triphone2]);
        }
    }

    for (auto cwnit = nodes.begin(); cwnit != nodes.end(); ++cwnit)
        if (cwnit->flags == 0) cwnit->flags |= NODE_CW;
}


}

