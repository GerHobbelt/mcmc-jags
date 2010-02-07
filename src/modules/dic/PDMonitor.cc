#include <config.h>
#include "PDMonitor.h"
#include <stdexcept>

using std::vector;
using std::logic_error;
using std::string;

namespace dic {

    PDMonitor::PDMonitor(StochasticNode const *snode)
	: Monitor("pD", snode)
    {
	if (snode->nchain() < 2) {
	    throw logic_error("PDMonitor needs at least 2 chains");
	}
    }

    unsigned int PDMonitor::nchain() const
    {
	return 1;
    }

    vector<unsigned int> PDMonitor::dim() const
    {
	return vector<unsigned int> (1,_values.size());
    }

    vector<unsigned int> PDMonitor::dim1() const
    {
	return vector<unsigned int> (1,1);
    }
 
    vector<double> const &PDMonitor::value(unsigned int chain) const
    {
	return _values;
    }

    void PDMonitor::reserve(unsigned int niter)
    {
	_values.reserve(_values.size() + niter);
    }

    SArray PDMonitor::dump() const
    {
	SArray ans(dim());
	ans.setValue(_values);
	
        vector<string> names(1,string("iteration"));
	ans.setDimNames(names);
	return ans;
    }

}
