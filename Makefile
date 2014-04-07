cxxflags = -O3 -march=native -std=gnu++0x -Wall
#cxxflags = -O3 -march=native -std=c++11 -Wall -Wno-sign-compare
#cxxflags = -g -O3 -march=native -std=gnu++0x -Wall -Wno-sign-compare
#cxxflags = -g -O3 -march=native -std=gnu++0x -Wall -Wno-sign-compare -fprofile-arcs -ftest-coverage
#cxxflags = -O0 -gddb -std=gnu++0x -Wall
#cxxflags = -O0 -g -std=gnu++0x -Wall -Wno-sign-compare
#cxxflags = -O0 -pg -std=gnu++0x -Wall -Wno-sign-compare

##################################################

progs = dgraph dgraph2 latables decode
progs_srcs = $(progs:=.cc)
progs_objs = $(progs:=.o)
srcs = conf.cc io.cc Ngram.cc Hmm.cc NowayHmmReader.cc DecoderGraph.cc LnaReaderCircular.cc Decoder.cc gutils.cc
objs = $(srcs:.cc=.o)

test_progs = runtests
test_progs_srcs = $(test_progs:=.cc)
test_progs_objs = $(test_progs:=.o)
test_srcs = graphtest.cc decodertest.cc
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

