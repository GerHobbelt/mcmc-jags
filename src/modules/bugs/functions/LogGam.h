#ifndef FUNC_LOGGAM_H_
#define FUNC_LOGGAM_H_

#include <function/ScalarFunc.h>

namespace bugs {

    /**
     * @short Log gamma function
     * <pre>
     * y <- loggam(x)
     * y = log(gamma(x)) for x > 0
     * </pre>
     */
    class LogGam:public ScalarFunc
    {
    public:
	LogGam ();
	double evaluateScalar(std::vector<double const *> const &args) const;
	bool checkScalarValue(std::vector<double const *> const &args) const;
    };

}

#endif /* FUNC_LOGGAM_H_ */
