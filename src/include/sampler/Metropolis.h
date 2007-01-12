#ifndef METROPOLIS_H_
#define METROPOLIS_H_

#include <sampler/Sampler.h>

/** 
 * @short Metropolis Hastings Sampler
 *
 * This is a base class for Metropolis Hastings samplers.  It provides
 * only basic infrastructure.
 *
 * The Metropolis Hasting sampler has its own value, which is distinct
 * from the value of the sampled nodes.  All member functions of the
 * Metropolis class work in terms of this value.  This is done to
 * allow parameter transformations: for example, if a node only takes
 * positive values, it may be more efficient for the sampler to work
 * on log-transformed values of the sampled node.
 *
 * A Subclass of Metropolis must provide an implementation of the
 * the virtual functions Metropolis##setValue and Metropolis##rescale.
 *
 * The Metropolis class provides no update member function. A subclass
 * of Metropolis must provide this. It should contain one or more
 * calls to Metropolis##propose, then calculate the acceptance
 * probability, and then call the function Metropolis##accept.
 */
class Metropolis : public Sampler
{
    const unsigned int _chain;
    const unsigned int _length;
    bool _adapt;
    double *_value;
    double *_last_value;
public:
    /**
     * Constructs a Metropolis Hastings sampler
     * 
     * @param nodes Vector of Stochastic Nodes to be sampled
     * @param graph Graph within which sampling is to take place
     * @param chain Chain number that the sampler will update
     * @param value Pointer to the beginning of an array of initial values.
     * @param length Length of initial value array
     */
    Metropolis(std::vector<StochasticNode *> const &nodes, 
	       Graph const &graph, unsigned int chain,
	       double const *value, unsigned int length);
    ~Metropolis();
    /**
     * Proposes a move to a new value. This function calls
     * Metropolis##setValue, and records the current value of the
     * sampler.
     *
     * @param value Pointer to the beginning of an array of values
     * @param length Length of value array
     */
    void propose(double const *value, unsigned int length);
    /**
     * Accept current value with probabilty p. If the current value is
     * not accepted, the sampler reverts to the value at the last
     * successful call to accept.
     *
     * @param rng Random number generator.
     * @param p Probability of accepting the current value.
     *
     * @returns success indicator
     */
    bool accept(RNG *rng, double p);
    /**
     * Returns the current value of the chain. 
     */
    double const *value() const;
    /**
     * Returns the length of the value array for the sampler
     */
    unsigned int length() const;
    /**
     * Rescales the proposal distribution. This function is called by
     * Metropolis##accept when the sampler is in adaptive
     * mode. Rescaling may optionally depend on the acceptance
     * probability, or empirical acceptance rate.
     *
     * @param p Acceptance probability
     * @param accept Success indicator for the current acceptance step.
     */
    virtual void rescale(double p, bool accept) = 0;
    /**
     * Sets the value of the sampler. Subclasses of Metropolis must
     * implement this function, which should transform the supplied
     * value on to a legitimate value for the sampled nodes, and then
     * call Sampler##setValue with the transformed values.
     *
     * @param value Pointer to the beginning of an array of values
     * @param length Length of value array
     */
    virtual void setValue(double const *value, unsigned int length) = 0;
    /**
     * Turns off adaptive mode
     */
    void adaptOff();
};

#endif /* METROPOLIS_H_ */
