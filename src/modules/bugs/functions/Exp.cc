#include <config.h>
#include "Exp.h"

#include <cmath>

using std::vector;
using std::exp;
using std::log;

Exp::Exp ()
  : InverseLinkFunc ("exp", "log")
{
}

double Exp::eval(vector<double const *> const &args) const
{
  return exp(*args[0]);
}

double Exp::link(double mu) const
{
  return log(mu);
}

double Exp::gradLink(double mu) const
{
  return 1/mu;
}
