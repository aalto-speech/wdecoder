-include Makefile.local

cxxflags = -O3 -DNDEBUG -std=gnu++0x -Wall -Wno-unused-function -I./util $(LOCAL_CXXFLAGS)
#cxxflags = -O0 -g -std=gnu++0x -Wall -Wno-unused-function

##################################################

util_srcs = util/conf.cc\
	util/io.cc\
	util/Hmm.cc\
	util/LnaReaderCircular.cc\
	util/Ngram.cc\
	util/NowayHmmReader.cc
util_objs = $(util_srcs:.cc=.o)

graph_srcs = graphs/DecoderGraph.cc\
	graphs/WordGraph.cc\
	graphs/SubwordGraph.cc\
	graphs/ConstrainedSWGraph.cc\
	graphs/LWBSubwordGraph.cc\
	graphs/RWBSubwordGraph.cc
graph_objs = $(graph_srcs:.cc=.o)

graph_progs = wgraph\
	swwgraph\
	swgraph\
	lwbswgraph\
	rwbswgraph

decoder_srcs = decoders/Decoder.cc\
	decoders/ClassDecoder.cc\
	decoders/ClassIPDecoder.cc\
	decoders/Segmenter.cc\
	decoders/Lookahead.cc
decoder_objs = $(decoder_srcs:.cc=.o)

decoder_progs = decode\
	class-decode\
	class-ip-decode\
	score\
	segment\
	lastates\
	cleanlex\
	lasc

test_srcs = test/wgraphtest.cc\
	test/swgraphtest.cc\
	test/swwgraphtest.cc\
	test/lwbswgraphtest.cc\
	test/rwbswgraphtest.cc\
	test/decodertest.cc
test_objs = $(test_srcs:.cc=.o)

test_progs = runtests

##################################################

.SUFFIXES:

all: $(graph_progs) $(decoder_progs) $(test_progs)

%.o: %.cc
	$(CXX) -c $(cxxflags) $< -o $@

$(graph_progs): $(util_objs) $(graph_objs)
	$(CXX) $(cxxflags) -o $@ graphs/$@.cc $(util_objs) $(graph_objs) -lz -I./graphs

$(decoder_progs): $(util_objs) $(graph_objs) $(decoder_objs)
	$(CXX) $(cxxflags) -o $@ decoders/$@.cc $(util_objs) $(graph_objs) $(decoder_objs)\
	 -lz -I./graphs -I./decoders

$(test_objs): %.o: %.cc $(test_srcs) $(util_objs) $(graph_objs) $(decoder_objs)
	$(CXX) -c $(cxxflags) $< -o $@ -I./graphs -I./decoders

$(test_progs): $(test_objs)
	$(CXX) $(cxxflags) -o $@ test/$@.cc $(test_objs) $(util_objs) $(graph_objs) $(decoder_objs)\
	 -lboost_unit_test_framework -lz -I./graphs -I./decoders

.PHONY: clean
clean:
	rm -f $(util_objs)\
	 $(graph_objs) $(graph_progs)\
	 $(decoder_objs)  $(decoder_progs)\
	 $(test_progs) $(test_objs) .depend

dep:
	$(CXX) -MM $(cxxflags) $(DEPFLAGS) $(all_srcs) > dep
include dep

