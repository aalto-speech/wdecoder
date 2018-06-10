#include "NgramDecoder.hh"
#include "ClassLookahead.hh"
#include "conf.hh"

using namespace std;

int main(int argc, char* argv[])
{
    conf::Config config;
    config("usage: clastates [OPTION...] PH LEXICON GRAPH CLASS_ARPA CMEMPROBS LASTATES\n")
    ('h', "help", "", "", "display help");
    config.default_parse(argc, argv);
    if (config.arguments.size() != 6) config.print_help(stderr, 1);

    try {
        NgramDecoder d;

        string phfname = config.arguments[0];
        cerr << "Reading hmms: " << phfname << endl;
        d.read_phone_model(phfname);

        string lexfname = config.arguments[1];
        cerr << "Reading lexicon: " << lexfname << endl;
        d.read_noway_lexicon(lexfname);

        string graphfname = config.arguments[2];
        cerr << "Reading graph: " << graphfname << endl;
        d.read_dgraph(graphfname);
        cerr << "node count: " << d.m_nodes.size() << endl;

        string classArpa = config.arguments[3];
        string cMemProbs = config.arguments[4];
        cerr << "Reading class bigram lookahead model: "
             << classArpa << "/" << cMemProbs << endl;
        ClassBigramLookahead cbgla(d, classArpa, cMemProbs);

        string lasfname = config.arguments[5];
        cerr << "Writing lookahead states: " << lasfname << endl;
        cbgla.writeStates(lasfname);
    } catch (string &e) {
        cerr << e << endl;
    }

    exit(0);
}
