#ifndef PD_MONITOR_H_
#define PD_MONITOR_H_

#include <model/Monitor.h>
#include <graph/StochasticNode.h>

#include <vector>

class StochasticNode;
class RNG;

namespace dic {

    class PDMonitor : public Monitor {
    protected:
	std::vector<double> _values;
    public:
	PDMonitor(std::vector<StochasticNode const *> const &nodes);
	unsigned int nchain() const;
	std::vector<unsigned int> dim() const;
	std::vector<unsigned int> dim1() const;
	std::vector<double> const &value(unsigned int chain) const;
	void reserve(unsigned int niter);
	SArray dump() const;
	bool poolChains() const;
	bool poolIterations() const;
    };

}

#endif /* PD_MONITOR_H_ */
