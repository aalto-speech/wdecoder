#include <cmath>
#include <cstring>
#include <cstdlib>
#include <set>

#include "NowayHmmReader.hh"

using namespace std;


void
NowayHmmReader::read_hmm(istream &in,
                         Hmm &hmm,
                         vector<HmmState> &hmm_states,
                         int &m_num_models)
{
    int hmm_id = 0;
    int num_states = 0;
    string label;
    vector<HmmState> states;

    in >> hmm_id;
    in >> num_states >> label;
    states.resize(num_states);

    // Real model ids for each state
    for (int s = 0; s < num_states; s++) {
        int model_index;
        in >> model_index;
        if (model_index + 1 > m_num_models)
            m_num_models = model_index + 1;
        states[s].model = model_index;
        if (model_index > -1 && model_index+1 > hmm_states.size())
            hmm_states.resize(model_index+1);
    }

    // Read transitions
    for (int s = 0; s < num_states; s++) {
        HmmState &state = states[s];
        int from;
        int to;
        int num_transitions = 0;
        float prob;

        in >> from >> num_transitions;
        state.transitions.resize(num_transitions);

        for (int t = 0; t < num_transitions; t++) {
            in >> to >> prob;
            if (to >= num_states || to < 1) {
                cerr << "hmm '" << label << "' has invalid transition" << endl;
                throw InvalidFormat();
            }
            state.transitions[t].target = to;
            state.transitions[t].log_prob = log10(prob);
        }

        if (states[s].model > -1) hmm_states[states[s].model] = state;
    }

    label.swap(hmm.label);
    states.swap(hmm.states);
}

void
NowayHmmReader::read(istream &in,
                     vector<Hmm> &m_hmms,
                     map<string, int> &m_hmm_map,
                     vector<HmmState> &m_hmm_states,
                     int &m_num_models)
{
    m_num_models = 0;
    istream::iostate old_state = in.exceptions();
    in.exceptions(in.badbit | in.failbit | in.eofbit);

    try {
        char buf[5];
        in.read(buf, 5);
        if (!in || strncmp(buf, "PHONE", 5))
            throw InvalidFormat();

        int num_hmms;
        in >> num_hmms;
        m_hmms.resize(num_hmms);
        for (int h = 0; h < num_hmms; h++) {
            // FIXME: should we check that hmm_ids read from stream match
            // with 'h'?
            read_hmm(in, m_hmms[h], m_hmm_states, m_num_models);
            m_hmm_map[m_hmms[h].label] = h;
        }

    }
    catch (exception &e) {
        in.exceptions(old_state);
        throw InvalidFormat();
    }

    in.exceptions(old_state);
}

void NowayHmmReader::read_durations(istream &in,
                                    vector<Hmm> &m_hmms,
                                    vector<HmmState> &m_hmm_states)
{
    istream::iostate old_state = in.exceptions();
    in.exceptions(in.badbit | in.failbit | in.eofbit);
    int version;
    int hmm_id;
    float a,b;
    float a0,a1,b0,b1;

    try {
        if (m_hmms.size() == 0)
        {
            cerr << "NowayHmmReader::read_durations(): Error: HMMs must be loaded before duration file!" << endl;
            exit(1);
        }
        in >> version;
        if (version != 1 && version != 2 && version != 3 && version != 4)
            throw InvalidFormat();
        if (version == 3 || version == 4)
        {
            vector<float> a_table;
            vector<float> b_table;
            int num_states, state_id;
            in >> num_states;
            if (version == 3)
                num_states++; // Used to be the index of the last state
            a_table.reserve(num_states);
            b_table.reserve(num_states);
            for (int i = 0; i < num_states; i++)
            {
                in >> state_id;
                if (state_id != i)
                    throw InvalidFormat();
                in >> a >> b;
                a_table.push_back(a);
                b_table.push_back(b);
            }
            for (int i = 0; i < m_hmms.size(); i++)
            {
                for (int s = 2; s < m_hmms[i].states.size(); s++)
                {
                    HmmState &state = m_hmms[i].states[s];
                    if (state.model >= num_states)
                        throw StateOutOfRange();
                    state.duration.set_parameters(a_table[state.model],
                                                  b_table[state.model]);
                    HmmState &state2 = m_hmm_states[state.model];
                    state2.duration.set_parameters(a_table[state.model],
                                                   b_table[state.model]);
                }
            }
        }
        else
        {
            for (int i = 0; i < m_hmms.size(); i++)
            {
                in >> hmm_id;
                if (hmm_id != i+1)
                    throw InvalidFormat();

                if (version == 2)
                {
                    in >> a >> b;
                    m_hmms[i].set_pho_dur_stat(a,b);
                }

                for (int s = 2; s < m_hmms[i].states.size(); s++)
                {
                    HmmState &state = m_hmms[i].states[s];
                    in >> a >> b;
                    state.duration.set_parameters(a,b);
                }

                if (version == 2)
                {
                    for (int s = 2; s < m_hmms[i].states.size(); s++)
                    {
                        HmmState &state = m_hmms[i].states[s];
                        in >> a0 >> a1 >> b0 >> b1;
                        state.duration.set_sr_parameters(a0,a1,b0,b1);
                    }
                }
            }
        }
    }
    catch (exception &e) {
        in.exceptions(old_state);
        throw InvalidFormat();
    }

    in.exceptions(old_state);
}
