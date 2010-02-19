#ifndef PD_MONITOR_H_
#define PD_MONITOR_H_

#include <model/Monitor.h>
#include <graph/StochasticNode.h>

#include <vector>

class StochasticNode;
class RNG;

namespace dic {

    class PoptMonitor : public Monitor {
	StochasticNode const *_snode;
	StochasticNode _repnode;
	const std::vector<RNG *> _rngs;
	unsigned int _nrep;
	std::vector<double> _weights; // weights for each chain
	std::vector<double> _values; // sampled values
    public:
	PoptMonitor(StochasticNode const *snode,
		    std::vector<RNG *> const &rngs, unsigned int nrep); 
	unsigned int nchain() const;
	std::vector<unsigned int> dim() const;
	std::vector<unsigned int> dim1() const;
	std::vector<double> const &value(unsigned int chain) const;
	void update();
	void reserve(unsigned int niter);
	SArray dump() const;
	bool poolChains() const;
	bool poolIterations() const;
    };

}

#endif /* PD_MONITOR_H_ */
