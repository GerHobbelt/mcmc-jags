#include <config.h>

#include "ConjugateDirichlet.h"

#include <rng/RNG.h>
#include <graph/MixtureNode.h>
#include <graph/StochasticNode.h>
#include <graph/AggNode.h>
#include <graph/NodeError.h>
#include <sampler/GraphView.h>
#include <module/ModuleError.h>

#include <set>
#include <vector>
#include <cmath>

#include <JRmath.h>

using std::vector;
using std::set;
using std::string;

namespace jags {
namespace bugs {

    Node const *findUniqueParent(Node const *node,
				 set<Node const *> const &nodeset)
    {
	/*
	  Utility function called by checkAggNode and checkMixNode. 

	  If node has a single parent in nodeset, then a pointer to
	  the parent is returned.  If there are multiple parents in
	  nodeset then a NULL pointer is returned.  

	  If no parents are in nodeset, a logic error is thrown.
	*/
	vector<Node const *> const &par = node->parents();
	Node const *param = 0;

	for (unsigned int i = 0; i < par.size(); ++i) {
	    if (nodeset.count(par[i])) {
		if (param) {
		    if (param != par[i]) return 0;
		}
		else {
		    param = par[i];
		}
	    }
	}
	if (param == 0) {
	    throwLogicError("Error in ConjugateDirichlet::canSample");
	}

	return param;
    }


static bool checkAggNode(AggNode const *anode, 
			 set<Node const *> const &nodeset)
{
    /* 
       Utility function called by ConjugateDirichlet::canSample

       The aggregate node (anode) must have only one parent in
       nodeset.  This parent must be embedded entirely within anode
       (i.e. we cannot take a subset of it) and the order of the
       elements cannot be permuted.
    */

    vector<Node const *> const &par = anode->parents();
    vector<unsigned int> const &off = anode->offsets();

    //Find unique parent
    Node const *param = findUniqueParent(anode, nodeset);
    if (param == 0) return false;

    //Check that parent is entirely contained in anode with offsets
    //in ascending order
    unsigned int j = 0;
    for (unsigned int i = 0; i < par.size(); ++i) {
	if (par[i] == param) {
	    if (off[i] != j) return false;
	    ++j;
	}
    }
    if (j != param->length()) {
	return false;
    }

    return true;
}

static bool checkMixNode(MixtureNode const *mnode, 
			 set<Node const *> const &nodeset)
{
    /*
      Utility function called by ConjugateDirichlet::canSample
      
      None of the indices may depend on nodeset. Among the other
      parents, only 1 may be in nodeset (although it may appear
      several times)
    */

    vector<Node const *> const &par = mnode->parents();

    //Check indices
    unsigned int nindex = mnode->index_size();
    for (unsigned int i = 0; i < nindex; ++i) {
	if (nodeset.count(par[i]))
	    return false;
    }

    //Find unique parent
    return findUniqueParent(mnode, nodeset) != 0;
}

bool ConjugateDirichlet::canSample(StochasticNode *snode, Graph const &graph)
{
    /*
      The ConjugateDirichlet sampler aims to be as general as
      possible, while minimizing the calculations that have to be done
      at each iteration.
      
      The stochastic children may be multinomial or categorical, and the
      deterministic children may include mixture nodes and aggregate
      nodes. However there are some restrictions that are implemented in
      the checkAggNode and checkMixNode functions.
      
      1) The deterministic descendents must form a tree, so that there is
      only one pathway from the sample node (snode) to each stochastic child.
      2) Aggregate nodes can only be used to embed snode (or one of its
      determistic descendants) in a larger node, not to take a subset.
      3) When a node X is embedded in an aggregate node Y, the order of the
      elements of X cannot be permuted in Y.

      These restrictions ensure that the _off values used by the
      update method are the same across chains and across iterations, and
      can therefore be calculated once in the constructor.

      Fascinating fact: Neither ddirch nor dcat requires a normalized
      probability argument.  This allows us to embed snode inside a
      larger aggregate node without worrying about what the other
      values are. For example:
      
      Y ~ dcat(p)
      p[1:3] ~ ddirch(alpha)
      p[4:5] <- W

      We can completely ignore the values of W (p[4] and p[5]). Normally
      these will be zero but we don't require it.
    */
    
    if(getDist(snode) != DIRCH)
	return false;
    
    if (isBounded(snode))
	return false;
    
    GraphView gv(snode, graph);
    vector<DeterministicNode*> const &dchild = gv.deterministicChildren();
    vector<StochasticNode const*> const &schild = gv.stochasticChildren();
    
    // Check stochastic children
    for (unsigned int i = 0; i < schild.size(); ++i) {
	vector<Node const *> const &param = schild[i]->parents();
	if (isBounded(schild[i])) {
	    return false; //Truncated
	}
	switch(getDist(schild[i])) {
	case CAT:
	    break;
	case MULTI:
	    if (gv.isDependent(param[1]))
		return false;
	    break;
	default:
	    return false;
	}
    }

    // Check deterministic descendants
    set<Node const *> nodeset;
    nodeset.insert(snode);
    for (unsigned int j = 0; j < dchild.size(); ++j) {
	if (MixtureNode const *m = dynamic_cast<MixtureNode const*>(dchild[j]))
	{
	    if (!checkMixNode(m, nodeset)) return false;	    
	}
	else if (AggNode const *a = dynamic_cast<AggNode const*>(dchild[j]))
	{
	    if (!checkAggNode(a, nodeset)) return false;
	}
	else {
	    return false;
	}
	nodeset.insert(dchild[j]);
    }
  
    return true;
}


static AggNode const *
getAggParent(Node const *node, set<Node const *> const &nodeset)
{
    /* 
       Search recursively for an AggNode ancestor node within nodeset.
       We assume that node has either 1 parent in nodeset or 0.
    */
    //FIXME: We can use _tree and _leaves
    vector<Node const*> const &par = node->parents();
    Node const *param = 0;
    for (unsigned int i = 0; i < par.size(); ++i) {
	if (nodeset.count(par[i])) {
	    param = par[i];
	    break;
	}
    }
    if (param == 0) {
	return 0;
    }
    
    AggNode const *anode = dynamic_cast<AggNode const *>(param);
    if (anode == 0) {
	anode = getAggParent(param, nodeset);
    }
    return anode;
}

static bool findMix(GraphView const *gv)
{
    /* 
       Utility function called by constructor. It returns true if any
       of the deterministic descendants are mixture nodes.

       When there are no mixture nodes, the update calculations can be
       simplified.  
       
       This static function is called by the constructor and the
       result is stored in the variable _mix.
    */
    vector<DeterministicNode*> const &dchild = gv->deterministicChildren();
    for (unsigned int i = 0; i < dchild.size(); ++i) {
	if (isMixture(dchild[i])) return true;
    }
    return false;
}

static vector<vector<unsigned int> > 
makeOffsets(GraphView const *gv, set<Node const *> const &nodeset) 
{
    vector<StochasticNode const *> const &schild = gv->stochasticChildren();
    unsigned int nchild = schild.size();
    vector<vector<unsigned int> > ans(nchild);

    for (unsigned int i = 0; i < nchild; ++i) {
	vector<unsigned int> off_i;
	if (AggNode const *anode = getAggParent(schild[i], nodeset)) {

	    vector<Node const *> const &par = anode->parents();
	    vector<unsigned int> const &off = anode->offsets();
	    
	    Node const *param = 0;
	    for (unsigned int j = 0; j < par.size(); ++j) {
		if (nodeset.count(par[j])) {
		    param = par[j];
		    break;
		}
	    }
	    if (param == 0) {
		throwLogicError("Error 3 in ConjugateDirichlet::canSample");
	    }

	    for (unsigned int j = 0; j < par.size(); ++j) {
		if (par[j] == param) {
		    off_i.push_back(off[j]);
		}
	    }
	    ans[i] = off_i;
	}
    }

    return ans;
}

    static vector<int> makeTree(GraphView const *gv)
    {
	/* 
	   If canSample is true then the nodes in the GraphView gv
	   form a tree with the sampled node as its root.

	   This function creates an integer vector "tree" denoting the
	   parents of the deterministic descendants: "tree[i] = j" for
	   j > 0 means that the unique parent of deterministic node
	   "i" in the GraphView is deterministic node "j".  If the
	   unique parent is the sampled node then we denote this by
	   "leaves[i] = -1".

	   This static function is called by the constructor, and
	   the result is stored in the variable _tree;
	*/
	vector<DeterministicNode*> const &dchild = gv->deterministicChildren();
	StochasticNode *snode = gv->nodes()[0];

	vector<int> tree(dchild.size(), -1);

	set<Node const *> nodeset;
	nodeset.insert(snode);

	for (unsigned int j = 0; j < dchild.size(); ++j) {
	    Node const *parent = findUniqueParent(dchild[j], nodeset);
	    if (parent == 0) {
		throwLogicError("Invalid tree in ConjugateDirichlet");
	    }
	    if (parent != snode) {
		unsigned int k = 0;
		for ( ; k < j; ++k) {
		    if (parent == dchild[k]) {
			tree[j] = k;
			break;
		    }
		}
		if (k == j) {
		    throwLogicError("Invalid tree in ConjugateDirichlet");
		}
	    }
	    nodeset.insert(dchild[j]);
	}	

	return tree;
    }

    static vector<int> makeLeaves(GraphView const *gv)
    {
	/* 
	   If canSample is true, the nodes in the GraphView gv form a
	   tree: the sampled node is the root and the stochastic
	   children are the leaves.

	   This function creates an integer vector "leaves" denoting
	   the parents of the stochastic children: "leaves[i] = j" for
	   j > 0 means that the unique parent of stochastic node "i"
	   in the GraphView is deterministic node "j".  If the unique
	   parent is the sampled node then we denote this by
	   "leaves[i] = -1".

	   This static function is called by the constructor, and
	   the result is stored in the variable _leaves;
	*/

	vector<StochasticNode const*> const &schild = gv->stochasticChildren();
	vector<DeterministicNode*> const &dchild = gv->deterministicChildren();
	StochasticNode *snode = gv->nodes()[0];

	set<Node const *> nodeset;
	nodeset.insert(snode);
	for (unsigned int k = 0; k < dchild.size(); ++k) {
	    nodeset.insert(dchild[k]);
	}

	vector<int> leaves(schild.size(), -1);
	for (unsigned int j = 0; j < schild.size(); ++j) {
	    Node const * parent = findUniqueParent(schild[j], nodeset);
	    if (parent != snode) {
		int k = dchild.size() - 1;
		for ( ; k >= 0; --k) {
		    if (parent == dchild[k]) {
			leaves[j] = k;
			break;
		    }
		}
		if (k < 0) {
		    throwLogicError("Leaf error in ConjugateDirichlet");
		}
	    }
	}	

	return leaves;
    }


ConjugateDirichlet::ConjugateDirichlet(GraphView const *gv)
    : ConjugateMethod(gv), _mix(findMix(gv)),
      _off(gv->stochasticChildren().size()),
      _tree(makeTree(gv)), _leaves(makeLeaves(gv))
{
    //Find out if we have any aggregate nodes in the deterministic
    //children
    vector<DeterministicNode *> const &dchild = gv->deterministicChildren();
    bool have_agg = false;
    for (unsigned int i = 0; i < dchild.size(); ++i) {
	if (dynamic_cast<AggNode const *>(dchild[i])) {
	    have_agg = true;
	    break;
	}
    }
    if (have_agg) {
	//We have aggregate nodes in the path and need to set up
	//offsets for the update function
	set<Node const*> nodeset;
	nodeset.insert(gv->nodes()[0]);
	for (unsigned int i = 0; i < dchild.size(); ++i) {
	    nodeset.insert(dchild[i]);
	}
	_off = makeOffsets(gv, nodeset);
	
	//Check offsets are the right length
	unsigned int slength = gv->nodes()[0]->length();
	for (unsigned int i = 0; i < _off.size(); ++i) {
	    if (!_off[i].empty() && _off[i].size() != slength) {
		throwLogicError("Invalid offsets if ConjugateDirichlet");
	    }
	}
    }
}

bool ConjugateDirichlet::isActiveLeaf(int i, unsigned int chain) const 
{
    /* 
       Helper function called by update. Returns true if the
       stochastic child with index i is "active", i.e. the path from
       the sampled node to stochastic child i is not blocked by a
       mixture node that is switched to another mixture component.
    */
    if (!_mix) return true; //There are no mixture nodes
    else return isActiveTree(_leaves[i], chain);
}

bool ConjugateDirichlet::isActiveTree(int i, unsigned int chain) const 
{
    /*
      Called by isActiveLeaf. See above for details.

      We traverse the tree of deterministic descendants back from the
      leaves until either we reach the sampled node (return true) or
      we find the path is blocked by a mixture node that is set to
      another mixture component (return false).
    */

    if (i == -1) {
	return true; //We have reached the sampled node
    }

    vector<DeterministicNode*> const &dchild = _gv->deterministicChildren();
    
    if (MixtureNode const *m = asMixture(dchild[i])) {
	Node const *active_parent = m->activeParent(chain);
	if (_tree[i] == -1) {
	    //active parent should be the sampled node
	    if (active_parent != _gv->nodes()[0]) {
		return false;
	    }
	}
	else {
	    //active parent should be the parent given by _tree
	    if (active_parent != dchild[_tree[i]]) {
		return false;
	    }
	}
    }
    
    return isActiveTree(_tree[i], chain);
}

void ConjugateDirichlet::update(unsigned int chain, RNG *rng) const
{
    StochasticNode *snode = _gv->nodes()[0];
    unsigned int size = snode->length();
    double *alpha = new double[size];
    double *xnew = new double[size];

    double const *prior = snode->parents()[0]->value(chain);
    for (unsigned long i = 0; i < size; ++i) {
	alpha[i] = prior[i];
    }

    vector<StochasticNode const*> const &schild = _gv->stochasticChildren();
    for (unsigned int i = 0; i < schild.size(); ++i) {
	int index = 0;
	double const *N = 0;

	if (isActiveLeaf(i, chain)) {
	    switch(_child_dist[i]) {
	    case MULTI:
		N = schild[i]->value(chain);
		if (_off[i].empty()) {
		    for (unsigned int j = 0; j < size; ++j) {
			alpha[j] += N[j];
		    }
		}
		else {
		    for (unsigned int j = 0; j < size; ++j) {
			alpha[j] += N[_off[i][j]];
		    }
		}
		break;
	    case CAT:
		index = static_cast<int>(*schild[i]->value(chain)) - 1;
		if (_off[i].empty()) {
		    alpha[index] += 1;
		}
		else {
		    for (unsigned int j = 0; j < size; ++j) {
			if (index == _off[i][j]) {
			    alpha[j] += 1;
			    break;
			}
		    }
		}
		break;
	    default:
		throwLogicError("Invalid distribution in ConjugateDirichlet");
	    }
	}
    }

    /* Check structural zeros */
    for (unsigned int i = 0; i < size; ++i) {
	if (prior[i] == 0 && alpha[i] != 0) {
	    throwNodeError(snode, "Invalid likelihood for Dirichlet distribution with structural zeros");
	}
    }
  
    /* 
       Draw Dirichlet sample by drawing independent gamma random
       variates and then normalizing
    */

    double xsum = 0.0;
    for (unsigned long i = 0; i < size; ++i) {
	if (alpha[i] > 0) {
	    xnew[i] = rgamma(alpha[i], 1, rng);
	    xsum += xnew[i];
	}
	else {
	    xnew[i] = 0;
	}
    }
    for (unsigned long i = 0; i < size; ++i) {
	xnew[i] /= xsum;
    }

    _gv->setValue(xnew, size, chain);

    delete [] xnew;
    delete [] alpha;
}

string ConjugateDirichlet::name() const
{
    return "ConjugateDirichlet";
}

}}
