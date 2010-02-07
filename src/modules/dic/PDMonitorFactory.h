#ifndef PD_MONITOR_FACTORY_H_
#define PD_MONITOR_FACTORY_H_

#include <model/MonitorFactory.h>

namespace dic {

    class PDMonitorFactory : public MonitorFactory
    {
      public:
	Monitor *getMonitor(std::string const &name, Range const &range,
			    BUGSModel *model, std::string const &type);
	std::vector<Node const*> defaultNodes(Model *model,
					      std::string const &type) const;
    };
    
}

#endif /* PD_MONITOR_FACTORY_H_ */
