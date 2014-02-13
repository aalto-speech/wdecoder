#ifndef NOWAYHMMREADER_HH
#define NOWAYHMMREADER_HH

#include <iostream>
#include <vector>
#include <map>

#include "Hmm.hh"

class NowayHmmReader {
public:

    static void read(std::istream &in,
                     std::vector<Hmm> &hmms,
                     std::map<std::string, int> &hmm_map,
                     std::vector<HmmState> &hmm_states,
                     int &num_models);
    static void read_durations(std::istream &in,
                               std::vector<Hmm> &hmms,
                               std::vector<HmmState> &hmm_states);

    struct InvalidFormat : public std::exception {
        virtual const char *what() const throw()
        {
            return "NowayHmmReader: invalid format";
        }
    };
    struct StateOutOfRange : public std::exception {
        virtual const char *what() const throw()
        {
            return "NowayHmmReader: state number of our range";
        }
    };

private:
    static void read_hmm(std::istream &in,
                         Hmm &hmm,
                         std::vector<HmmState> &hmm_states,
                         int &num_models);
};

#endif /* NOWAYHMMREADER_HH */
