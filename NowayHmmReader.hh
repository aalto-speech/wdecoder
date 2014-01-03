#ifndef NOWAYHMMREADER_HH
#define NOWAYHMMREADER_HH

#include <iostream>
#include <vector>
#include <map>

#include "Hmm.hh"

class NowayHmmReader {
public:

    static void read(std::istream &in,
                     std::vector<Hmm> &m_hmms,
                     std::map<std::string, int> &m_hmm_map,
                     int &m_num_models);
    static void read_durations(std::istream &in,
                               std::vector<Hmm> &m_hmms);

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
    static void read_hmm(std::istream &in, Hmm &hmm, int &m_num_models);
};

#endif /* NOWAYHMMREADER_HH */
