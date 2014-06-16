#ifndef DEFS_HH
#define DEFS_HH

#ifndef nullptr
#include <cstdlib>
#define nullptr NULL
#endif

typedef unsigned int sw_node_idx_t;
typedef unsigned int node_idx_t;

#define START_NODE   0
#define END_NODE     1

#define NODE_FAN_OUT_DUMMY              0x01
#define NODE_FAN_IN_DUMMY               0x02
#define NODE_CW                         0x04
#define NODE_SILENCE                    0x08
#define NODE_INITIAL                    0x10
#define NODE_BIGRAM_LA_TABLE            0x20
#define NODE_DECODE_START               0x40
#define NODE_SUBWORD_START              0x80

#endif /* DEFS_HH */
