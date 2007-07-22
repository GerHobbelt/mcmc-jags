#ifndef FUNC_GREATER_THAN_H_
#define FUNC_GREATER_THAN_H_

#include "Infix.h"

/**
 * @short Numeric comparison
 * <pre>
 * y <- a > b
 * y = 1 if a > b
 *     0 if a <= b
 * </pre>
 */
class GreaterThan:public Infix
{
public:
    GreaterThan ();
    double eval(std::vector<double const *> const &args) const;
    /** Returns true */
    bool isDiscreteValued(std::vector<bool> const &mask) const;
};

#endif /* FUNC_GREATER_THAN_H_ */
