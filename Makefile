cxxflags = -O3 -march=native -DNDEBUG -std=gnu++0x -Wall
#cxxflags = -O0 -gddb -std=gnu++0x -Wall
#cxxflags = -O0 -g -std=gnu++0x -Wall
#cxxflags = -O0 -pg -std=gnu++0x -Wall

##################################################

progs = wgraph swwgraph swgraph decode score segment lastates cleanlex
progs_srcs = $(progs:=.cc)
progs_objs = $(progs:=.o)
srcs = conf.cc io.cc Ngram.cc Hmm.cc NowayHmmReader.cc DecoderGraph.cc gutils.cc GraphBuilder2.cc SubwordGraphBuilder.cc WordGraphBuilder.cc LnaReaderCircular.cc Decoder.cc Segmenter.cc Lookahead.cc 
objs = $(srcs:.cc=.o)

test_progs = runtests
test_progs_srcs = $(test_progs:=.cc)
test_progs_objs = $(test_progs:=.o)
test_srcs = graphtest.cc
#test_srcs = graphtest.cc swgraphtest.cc decodertest.cc
#test_srcs = decodertest.cc
test_objs = $(test_srcs:.cc=.o)

##################################################

.SUFFIXES:

all: $(progs) $(test_progs)

%.o: %.cc
	$(CXX) -c $(cxxflags) $< -o $@

$(progs): %: %.o $(objs)
	$(CXX) $(cxxflags) $< -o $@ $(objs)

%: %.o $(objs)
	$(CXX) $(cxxflags) $< -o $@ $(objs)

$(test_progs): %: %.o $(objs) $(test_objs)
	$(CXX) $(cxxflags) $< -o $@ $(objs) $(test_objs) -lcppunit

test_objs: $(test_srcs)

test_progs: $(objs) $(test_objs)

test: $(test_progs)

.PHONY: clean
clean:
	rm -f $(objs) *.o $(progs) $(progs_objs) $(test_progs) $(test_progs_objs) $(test_objs) .depend *~

dep:
	$(CXX) -MM $(cxxflags) $(DEPFLAGS) $(all_srcs) > dep
include dep

