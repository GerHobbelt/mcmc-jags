#ifndef TRANSPOSE_H_
#define TRANSPOSE_H_

#include <function/Function.h>

namespace bugs {

    class Transpose : public Function
    {
    public:
	Transpose();
	void evaluate(double *value, std::vector<double const *> const &args,
		      std::vector<unsigned int> const &lengths,
		      std::vector<std::vector<unsigned int> > const &dims) 
	    const;
	std::vector<unsigned int> 
	    dim(std::vector<std::vector<unsigned int> > const &dims) const;
	bool checkParameterDim(std::vector<std::vector<unsigned int> > const &dims) const;
	bool isLinear(std::vector<bool> const &mask,
		      std::vector<bool> const &isfixed) const;
	bool isScale(unsigned int index, std::vector<bool> const &mask) const;
    };

}

#endif /* TRANSPOSE_H_ */
