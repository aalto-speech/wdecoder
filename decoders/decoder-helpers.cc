#include <algorithm>
#include <thread>
#include <vector>
#include <typeinfo>

#include "Decoder.hh"
#include "NgramDecoder.hh"
#include "ClassDecoder.hh"
#include "ClassIPDecoder.hh"
#include "WordSubwordDecoder.hh"
#include "conf.hh"

using namespace std;

void join(vector<string> &lnafnames,
          vector<Recognition*> &recognitions,
          vector<RecognitionResult*> &results,
          vector<thread*> &threads,
          TotalRecognitionStats &total,
          ostream &resultf,
          ostream &logf,
          SimpleFileOutput *nbest = nullptr,
          int nbest_num_hypotheses = 10000)
{
    for (int i=0; i<(int)threads.size(); i += 1) {
        threads[i]->join();
        logf << endl << "recognizing: " << lnafnames[i] << endl;
        resultf << lnafnames[i] << ":" << results[i]->best_result.result << endl;
        results[i]->print_file_stats(logf);
        total.accumulate(*results[i]);
        if (nbest != nullptr) {
            vector<RecognitionResult::Result> nbest_results
                = results[i]->get_nbest_results();
            int hypos_to_write = std::min((int)nbest_results.size(), nbest_num_hypotheses);
            cerr << "\tNumber of n-best hypotheses written: " << hypos_to_write << endl;
            for (int h=0; h<hypos_to_write; h++)
                *nbest << lnafnames[i]
                       << " " << nbest_results[h].total_lp
                       << " " << nbest_results[h].total_am_lp
                       << " " << nbest_results[h].total_lm_lp
                       << " " << std::count(nbest_results[h].result.begin(), nbest_results[h].result.end(), ' ')
                       << nbest_results[h].result << "\n";
        }
        delete recognitions[i];
        delete results[i];
        delete threads[i];
    }
    lnafnames.clear();
    recognitions.clear();
    results.clear();
    threads.clear();
}


Recognition*
get_recognition(Decoder *decoder) {
    if (dynamic_cast<NgramDecoder*>(decoder)) {
        return new NgramRecognition(*dynamic_cast<NgramDecoder*>(decoder));
    } else if (dynamic_cast<ClassDecoder*>(decoder)) {
        return new ClassRecognition(*dynamic_cast<ClassDecoder*>(decoder));
    }  else if (dynamic_cast<ClassIPDecoder*>(decoder)) {
        return new ClassIPRecognition(*dynamic_cast<ClassIPDecoder*>(decoder));
    } else if (dynamic_cast<WordSubwordDecoder*>(decoder)) {
        return new WordSubwordRecognition(*dynamic_cast<WordSubwordDecoder*>(decoder));
    } else {
        cerr << "Decoder type not supported: " << typeid(*decoder).name() << endl;
        exit(1);
    }
}


void
recognize_lnas(Decoder &d,
               conf::Config &config,
               string lnalistfname,
               ostream &resultf,
               ostream &logf,
               SimpleFileOutput *nbest = nullptr)
{
    ifstream lnalistf(lnalistfname);
    string lnafname;
    TotalRecognitionStats total;

    logf << "number of threads: " << config["num-threads"].get_int() << endl;
    logf << std::boolalpha;
    d.print_config(logf);

    int num_threads = config["num-threads"].get_int();
    vector<string> lna_fnames;
    vector<Recognition*> recognitions;
    vector<RecognitionResult*> results;
    vector<std::thread*> threads;
    while (getline(lnalistf, lnafname)) {
        if (!lnafname.length()) continue;

        if ((int)recognitions.size() < num_threads) {
            lna_fnames.push_back(lnafname);
            recognitions.push_back(get_recognition(&d));
            results.push_back(new RecognitionResult());
            thread *thr = new thread(&Recognition::recognize_lna_file,
                                     recognitions.back(),
                                     lnafname,
                                     std::ref(*results.back()),
                                     config["nbest"].specified,
                                     config["nbest-beam"].get_double(),
                                     config["nbest-num-hypotheses"].get_int());
            threads.push_back(thr);
        }

        if ((int)recognitions.size() == num_threads)
            join(lna_fnames, recognitions, results, threads, total, resultf, logf, nbest, config["nbest-num-hypotheses"].get_int());
    }
    lnalistf.close();
    join(lna_fnames, recognitions, results, threads, total, resultf, logf, nbest, config["nbest-num-hypotheses"].get_int());
    if (total.num_files > 1) total.print_stats(logf);
}
