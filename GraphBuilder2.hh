#ifndef GRAPH_BUILDER_2_HH
#define GRAPH_BUILDER_2_HH

#include "DecoderGraph.hh"


namespace graphbuilder2 {

static void read_word_segmentations(std::string segfname,
                                    std::vector<std::pair<std::string, std::vector<std::string> > > &word_segs);

};

#endif /* GRAPH_BUILDER_2_HH */
