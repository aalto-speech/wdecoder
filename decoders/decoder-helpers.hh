#ifndef DECODER_HELPERS_HH
#define DECODER_HELPERS_HH

#include <string>
#include <thread>
#include <vector>

#include "Decoder.hh"
#include "conf.hh"
#include "io.hh"

void join(std::vector<std::string> &lnafnames,
          std::vector<Recognition*> &recognitions,
          std::vector<RecognitionResult*> &results,
          std::vector<std::thread*> &threads,
          TotalRecognitionStats &total,
          std::ostream &resultf,
          std::ostream &logf,
          SimpleFileOutput *nbest = nullptr,
          int nbest_num_hypotheses = 10000);

Recognition* get_recognition(Decoder* decoder);

void recognize_lnas(Decoder &d,
                    conf::Config &config,
                    std::string lnalistfname,
                    std::ostream &resultf,
                    std::ostream &logf,
                    SimpleFileOutput *nbest = nullptr);

#endif /* DECODER_HELPERS_HH */
