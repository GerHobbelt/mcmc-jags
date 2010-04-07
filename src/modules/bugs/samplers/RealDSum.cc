#include <config.h>
#include <distribution/Distribution.h>
#include <graph/StochasticNode.h>
#include <graph/Graph.h>
#include <sampler/GraphView.h>
#include <rng/RNG.h>

#include "RealDSum.h"

#include <cfloat>
#include <climits>
#include <stdexcept>
#include <set>
#include <algorithm>
#include <cmath>

#include <JRmath.h>

using std::set;
using std::vector;
using std::invalid_argument;
using std::max;
using std::min;
using std::exp;
using std::string;
using std::floor;

static vector<double> nodeValues(GraphView const *gv, unsigned int chain)
{
    unsigned int n = gv->nodes().size();
    vector<double> ans(n);
    gv->getValue(ans, chain);

    //Correct initial value of sampled nodes to conform to the
    //constraint that they sum to dsum

    double delta = gv->stochasticChildren()[0]->value(chain)[0];
    for (unsigned int i = 0; i < n; ++i) {
	delta -= ans[i];
    }
    delta /= n;
    for (unsigned int i = 0; i < n; ++i) {
	ans[i] += delta;
    }
    gv->setValue(ans, chain);

    return(ans);
}

RealDSum::RealDSum(GraphView const *gv, unsigned int chain,
		   unsigned int nrep)
    : RWMetropolis(nodeValues(gv, chain), 0.1), 
      _gv(gv), _chain(chain), _nrep(nrep)
{
}

bool RealDSum::canSample(vector<StochasticNode *> const &nodes,
			 Graph const &graph)
{
    if (nodes.size() < 2)
	return false;

    for (unsigned int i = 0; i < nodes.size(); ++i) {
	if (!graph.contains(nodes[i]))
	    return false;

	// Nodes must be scalar ...
	if (nodes[i]->length() != 1)
	    return false;
    
	// Full rank
	if (nodes[i]->df() != 1)
	    return false;

	// Cannot be discrete
	if (nodes[i]->isDiscreteValued())
	    return false;
    }

    /* Nodes must be direct parents of a single observed stochastic
       node with distribution DSum  */

    GraphView gv(nodes, graph);
    vector<DeterministicNode*> const &dchild = gv.deterministicChildren();
    vector<StochasticNode const*> const &schild = gv.stochasticChildren();

    if (!dchild.empty())
	return false;
    if (schild.size() != 1)
	return false;
    if (!schild[0]->isObserved())
	return false;
    if (schild[0]->distribution()->name() != "dsum")
	return false;
    if (schild[0]->parents().size() != nodes.size())
	return false;

    // And so, their work was done...
    return true;
}

void RealDSum::step(vector<double> &value, double s, RNG *rng) const
{
    for (unsigned int r = 0; r < _nrep; ++r) {

	//Randomly draw two components of the vector
	int n = value.size();
	int i = static_cast<int>(rng->uniform() * n);
	if (i == n) --i; 
	int j = static_cast<int>(rng->uniform() * (n - 1));
	if (j == n - 1) --j;
	if (j >= i) ++j;
	
	//Modify the chosen components while keeping the sum constant
	double eps = rng->normal() * s;
	value[i] += eps;
	value[j] -= eps;

    }
}

string RealDSum::name() const
{
    return "RealDSum";
}

double RealDSum::logDensity() const
{
    return _gv->logFullConditional(_chain);
}

void RealDSum::setValue(vector<double> const &value)
{
    _gv->setValue(value, _chain);
}

void RealDSum::getValue(vector<double> &value) const
{
    _gv->getValue(value, _chain);
}
