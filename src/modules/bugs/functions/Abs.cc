#include <config.h>
#include "Abs.h"

#include <cmath>

using std::vector;
using std::fabs;

namespace bugs {

    Abs::Abs ():ScalarFunc ("abs", 1)
    {
    }

    double Abs::evaluateScalar(vector<double const *> const &args) const
    {
	return fabs(*args[0]);
    }


    bool Abs::isDiscreteValued(vector<bool> const &mask) const
    {
	return mask[0];
    }
}
