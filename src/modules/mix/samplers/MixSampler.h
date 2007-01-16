#ifndef MIX_SAMPLER_H_
#define MIX_SAMPLER_H_

#include <sampler/Metropolis.h>

class MixSampler : public Metropolis
{
    const unsigned int _chain;
    const unsigned int _nlevels;
    const unsigned int _nstep;
    double _log_min_power;
    const double _target_prob;
    double *_value;
    double *_lower;
    double *_upper;
    std::vector<double> _prob;
    std::vector<unsigned int> _n;
    std::vector<double> _lscale;
    std::vector<bool> _p_over_target;
    unsigned int _global_n;
    bool _global_p_over_target;
public:
    MixSampler(std::vector<StochasticNode *> const &snodes, Graph const &graph,
	       unsigned int chain, unsigned int nlevels=50, 
               double min_power=0.5, double target_prob = 0.234);
    ~MixSampler();
    void update(RNG *rng);
    void propose(double const *value, unsigned int length);
    void getValue(double *value, unsigned int length);
    void rescale(double prob, bool accept);
    static bool canSample(std::vector<StochasticNode *> snodes, Graph const &graph);
    double const *value() const;
};

#endif /* MIX_SAMPLER_H_ */
