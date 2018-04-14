-include Makefile.local

cxxflags = -O3 -DNDEBUG -std=gnu++0x -Wall -Wno-unused-function -I./util $(LOCAL_CXXFLAGS)
#cxxflags = -O0 -g -std=gnu++0x -Wall -Wno-unused-function -I./util $(LOCAL_CXXFLAGS)

##################################################

util_srcs = util/conf.cc\
	util/io.cc\
	util/Hmm.cc\
	util/LnaReaderCircular.cc\
	util/Ngram.cc\
	util/ClassNgram.cc\
	util/NowayHmmReader.cc\
	util/DynamicBitset.cc\
	util/QuantizedLogProb.cc
util_objs = $(util_srcs:.cc=.o)

graph_srcs = graphs/DecoderGraph.cc\
	graphs/WordGraph.cc\
	graphs/SubwordGraph.cc\
	graphs/ConstrainedSWGraph.cc\
	graphs/LWBSubwordGraph.cc\
	graphs/RWBSubwordGraph.cc\
	graphs/LRWBSubwordGraph.cc
graph_objs = $(graph_srcs:.cc=.o)

graph_progs = wgraph\
	swwgraph\
	swgraph\
	lwbswgraph\
	rwbswgraph\
	lrwbswgraph
graph_progs_srcs = $(addsuffix .cc,$(addprefix graphs/,$(graph_progs)))

decoder_srcs = decoders/Decoder.cc\
	decoders/Lookahead.cc\
	decoders/ClassLookahead.cc\
	decoders/NgramDecoder.cc\
	decoders/ClassDecoder.cc\
	decoders/ClassIPDecoder.cc\
	decoders/WordSubwordDecoder.cc\
	decoders/Segmenter.cc

decoder_objs = $(decoder_srcs:.cc=.o)

decoder_progs = decode\
	class-decode\
	class-ip-decode\
	wsw-decode\
	score\
	segment\
	lastates\
	cleanlex\
	lasc\
	clastates
decoder_progs_srcs = $(addsuffix .cc,$(addprefix decoders/,$(decoder_progs)))

test_srcs = test/wgraphtest.cc\
	test/swgraphtest.cc\
	test/swwgraphtest.cc\
	test/lwbswgraphtest.cc\
	test/rwbswgraphtest.cc\
	test/lrwbswgraphtest.cc\
	test/lookaheadtest.cc\
	test/classlatest.cc\
	test/bitsettest.cc\
	test/quantizedlptest.cc
test_objs = $(test_srcs:.cc=.o)

test_progs = runtests

##################################################

.SUFFIXES:

all: $(graph_progs) $(decoder_progs) $(test_progs)
#all: $(graph_progs) $(decoder_progs)

%.o: %.cc
	$(CXX) -c $(cxxflags) $< -o $@

$(graph_progs): $(graph_progs_srcs) $(util_objs) $(graph_objs)
	$(CXX) $(cxxflags) -o $@ graphs/$@.cc $(util_objs) $(graph_objs) -lz -I./graphs

$(decoder_progs): $(decoder_progs_srcs) $(util_objs) $(graph_objs) $(decoder_objs)
	$(CXX) $(cxxflags) -o $@ decoders/$@.cc $(util_objs) $(graph_objs) $(decoder_objs)\
	 -lz -pthread -I./graphs -I./decoders

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

