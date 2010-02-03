#ifndef KL_PD_MONITOR_H_
#define KL_PD_MONITOR_H_

//#include <vector>

#include "PDMonitor.h"
#include "KL.h"

class StochasticNode;
class RNG;

namespace dic {

    class KLPDMonitor : public PDMonitor {
	KL const *_kl;
	std::vector<std::vector<double const *> > _par;
    public:
	KLPDMonitor(StochasticNode const *snode, KL const *kl);
        ~KLPDMonitor();
	void update();
    };

}

#endif /* KL_PD_MONITOR_H_ */
