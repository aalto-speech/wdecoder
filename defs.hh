#ifndef DEFS_HH
#define DEFS_HH

#include <sstream>
#include <string>
#ifndef nullptr
#include <cstdlib>
#define nullptr NULL
#endif

typedef unsigned int sw_node_idx_t;
typedef unsigned int node_idx_t;

#define SIL_CTXT '_'

#define START_NODE   0
#define END_NODE     1

#define NODE_FAN_OUT_DUMMY              0x0001
#define NODE_FAN_IN_DUMMY               0x0002
#define NODE_CW                         0x0004
#define NODE_SILENCE                    0x0008
#define NODE_INITIAL                    0x0010
#define NODE_BIGRAM_LA_TABLE            0x0020
#define NODE_DECODE_START               0x0040
#define NODE_LM_LEFT_LIMIT              0x0080
#define NODE_LM_RIGHT_LIMIT             0x0100
#define NODE_TAIL                       0x0200

static int str2int(std::string str) {
    int val;
    std::istringstream numstr(str);
    numstr >> val;
    return val;
}


static std::string int2str(int a)
{
    std::ostringstream temp;
    temp<<a;
    return temp.str();
}

#endif /* DEFS_HH */
