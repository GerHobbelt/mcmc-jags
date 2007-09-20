#ifndef FINITE_METHOD_H_
#define FINITE_METHOD_H_

#include <sampler/DensityMethod.h>

namespace base {

/**
 * Sampler for discrete distributions with support on a finite set.
 */
    class FiniteMethod : public DensityMethod {
	int _lower, _upper;
    public:
	FiniteMethod(StochasticNode const *snode);
	void update(RNG *rng);
	/**
	 * FiniteMethod has no adaptive mode. This function simply
	 * returns true
	 */
	bool adaptOff();
	static bool canSample(StochasticNode const *snode);
    };

}

#endif /* FINITE_METHOD_H_ */
