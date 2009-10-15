#include <config.h>

#include "GLMFactory.h"
#include "GLMSampler.h"

#include <graph/GraphMarks.h>
#include <graph/Graph.h>
#include <graph/StochasticNode.h>
#include <graph/DeterministicNode.h>
#include <graph/LinkNode.h>
#include <distribution/Distribution.h>
#include <sampler/Linear.h>
#include <sampler/Updater.h>

#include <set>
#include <map>
#include <algorithm>

using std::set;
using std::vector;
using std::map;
using std::stable_sort;
using std::string;


/*
  Aggregates candidate Nodes into a joint linear model. 
  
  Although each candidate node has a conjugate normal distribution,
  together they may not form a valid linear model. We therefore test
  the validity of the joint linear model before aggregating.
  
  returns 2 if the candidate can be aggregated with the sample nodes
  returns 1 if the candidate cannot be aggregated because there is
            overlap but the joint model is non-linear
  returns 0 if there is no overlap

  If return value is 2, the arguments sample_nodes and stochastic_children
  are augmented.
*/
static bool aggregateLinear(Updater const *candidate,
			    vector<StochasticNode*> &sample_nodes,
			    set<StochasticNode const *> &stochastic_children,
			    Graph const &graph)
{

    // Check that there is some overlap in stochastic children between
    // candidate node and current set.  

    vector<StochasticNode const*> const &candidate_children = 
	candidate->stochasticChildren();

    bool overlap = false;
    for (unsigned int i = 0; i < candidate_children.size(); ++i) {
	if (stochastic_children.count(candidate_children[i]) > 0) {
	    overlap = true;
	    break;
	}
    }
    
    if (overlap) {
	sample_nodes.push_back(candidate->nodes()[0]); 
	stochastic_children.insert(candidate_children.begin(), 
				   candidate_children.end());
    }
    
    return overlap;
}

struct less_updater {
    /* 
       Comparison operator for updaters which sorts them in
       reverse order of the number of stochastic children
    */

    bool operator()(Updater const *x, Updater const *y) const {
	return (x->stochasticChildren().size() > 
		y->stochasticChildren().size());
	
    };
};

namespace glm {
    
    GLMFactory::GLMFactory(string const &name)
	: _name(name)
    {}

    bool GLMFactory::checkDescendants(Updater const *updater) const
    {
	// Create a set of nodes containing snode and its
	// deterministic descendants
	set<Node const*> paramset;
	paramset.insert(updater->nodes().begin(),
			updater->nodes().end());
	paramset.insert(updater->deterministicChildren().begin(), 
			updater->deterministicChildren().end());

	// Check stochastic children
	vector<StochasticNode const*> const &stoch_nodes = 
	    updater->stochasticChildren();
	bool have_link = false;
	for (unsigned int i = 0; i < stoch_nodes.size(); ++i) {
	    if (isBounded(stoch_nodes[i])) {
		return false; //Truncated outcome variable
	    }
	    if (!checkOutcome(stoch_nodes[i])) {
		return false; //Invalid outcome distribution
	    }
	    vector<Node const *> const &param = stoch_nodes[i]->parents();
	    //Check for link functions
	    LinkNode const *lnode = dynamic_cast<LinkNode const*>(param[0]);
	    if (lnode) {
		have_link = true;
		if (!checkLink(lnode->link()))
		    return false;
	    }
	    //Check that other parameters do not depend on snode	    
	    for (unsigned int j = 1; j < param.size(); ++j) {
		if (paramset.count(param[j])) {
		    return false;
		}
	    }
	}

	// Check linearity of deterministic descendants
	if (!checkLinear(updater, false, have_link))
	    return false;

	return true;
    }


    Updater * 
    GLMFactory::canSample(StochasticNode *snode, Graph const &graph) const
    {
	/*
	  Check whether whether an individual node can be sampled 

	  Returns a newly allocated Updater if successful, otherwise
	  zero.
	*/

	string dname = snode->distribution()->name();
	if (dname != "dnorm" && dname != "dmnorm")
	    return 0; //Must have normal prior

	if (isBounded(snode))
	    return 0; //Cannot have bounded prior

	Updater *updater = new Updater(snode, graph);
	if (!checkDescendants(updater)) {
	    delete updater;
	    return 0;
	}
	else {
	    return updater;
	}
    }

    GLMFactory::~GLMFactory()
    {}
    
    Sampler * 
    GLMFactory::makeSampler(set<StochasticNode*> const &nodes, 
			    Graph const &graph) const
    {
	/* 
	   Find candidate nodes that could be in a linear model.
	   Keep track of the number of stochastic children
	*/
	vector<Updater*> candidates;
	for (set<StochasticNode*>::const_iterator p = nodes.begin();
	     p != nodes.end(); ++p)
	{
	    Updater *up = canSample(*p, graph);
	    if (up) {
		candidates.push_back(up);
	    }
	}
	if (candidates.empty())
	    return 0;

	//Sort candidates in order of decreasing number of stochastic children
	stable_sort(candidates.begin(), candidates.end(), less_updater());

	//Now try to aggregate nodes into a joint linear model
	unsigned int Nc = candidates.size();
	vector<bool> keep(Nc, false);
	Updater *updater = 0;
	for (unsigned int i = 0; i < Nc; ++i) {
	    
	    keep[i] = true;

	    vector<StochasticNode*> sample_nodes(1, candidates[i]->nodes()[0]);
	    set<StochasticNode const *> stoch_children;
	    stoch_children.insert(candidates[i]->stochasticChildren().begin(),
				  candidates[i]->stochasticChildren().end());

	    //Find a joint linear model.
	    bool loop = false;
	    do {
		loop = false;
		for (unsigned int j = i+1; j < candidates.size(); ++j) {

		    if (!keep[j]) {
			keep[j] = aggregateLinear(candidates[j], 
						  sample_nodes, 
						  stoch_children, 
						  graph);
			if (keep[j])
			    loop = true;
		    }
		}
	    } while (loop);

	    if (sample_nodes.size() > 1) {
		updater = new Updater(sample_nodes, graph);
		if (checkLinear(updater, false, true)) {
		    break;
		}
		else {
		    delete updater; updater = 0;
		}
	    }
	    
	    for (unsigned int j = i; j < candidates.size(); ++j) {
		keep[j] = false;
	    }
	}

	vector<Updater*> sub_updaters;
	for (unsigned int i = 0; i < Nc; ++i) {
	    if (keep[i]) {
		sub_updaters.push_back(candidates[i]);
	    }
	    else {
		delete candidates[i];
	    }
	}

	if (!sub_updaters.empty()) {
		
	    unsigned int Nch = nchain(updater);
	    vector<SampleMethod*> methods(Nch, 0);
		
	    vector<Updater const*> const_sub_updaters(sub_updaters.size());
	    for (unsigned int i = 0; i < sub_updaters.size(); ++i) {
		const_sub_updaters[i] = sub_updaters[i];
	    }
	    for (unsigned int ch = 0; ch < Nch; ++ch) {
		methods[ch] = newMethod(updater, const_sub_updaters, ch);
	    }
	    return new GLMSampler(updater, sub_updaters, methods);
	}
	else {
	    return 0;
	}
    }

    string const &GLMFactory::name() const
    {
	return _name;
    }
}

