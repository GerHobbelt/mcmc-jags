#ifndef DIRICHLET_CAT_H_
#define DIRICHLET_CAT_H_

#include <sampler/SampleMethod.h>

#include <map>
#include <vector>

namespace jags {

    class MixtureNode;
    class Node;
    class GraphView;

    namespace mix {
	/**
	 * @short Block sampler for Dirichlet nodes in a mixture model.  
	 *
	 */
	class DirichletCat : public SampleMethod {
	    GraphView const *_gv;
	    std::map<Node const*, std::vector<double> > _parmap;
	    std::vector<MixtureNode const *> _mixparents;
	    unsigned int _chain;
	    unsigned int _size;
	  public:
	    DirichletCat(GraphView const *gv, unsigned int chain);
	    void update(RNG *rng);
	    bool isAdaptive() const;
	    void adaptOff();
	    bool checkAdaptation() const;
	    std::string name() const;
	    static bool canSample(GraphView const *gv);
	};
    } 
}

#endif /* DIRICHLET_CAT_H_ */
