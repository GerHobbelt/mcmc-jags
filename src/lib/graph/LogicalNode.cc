#include <config.h>
#include <graph/LogicalNode.h>
#include <function/Function.h>
#include <graph/NodeError.h>
#include <graph/GraphMarks.h>
#include <graph/Graph.h>
#include <util/dim.h>

#include <stdexcept>
#include <vector>
#include <string>
#include <math.h>

using std::vector;
using std::string;
using std::set;
using std::logic_error;
using std::runtime_error;

static vector<unsigned int> mkDim(Function const *func, 
				  vector<Node const *> const &parents)
{
  /* 
     Calculates dimension of logical node as a function of its
     parameters.
  */

  vector<vector<unsigned int> > parameter_dims(parents.size());
  for (unsigned int j = 0; j < parents.size(); ++j) {
    parameter_dims[j] = parents[j]->dim();
  }
  if (!func) {
    throw logic_error("NULL function in LogicalNode constructor");
  }
  if (!func->checkParameterLength(parameter_dims.size())) {
    //FIXME: logic_error or runtime_error?
    throw runtime_error(string("Incorrect number of parameters for function ")
		      + func->name());
  }
  if (!func->checkParameterDim(parameter_dims)) {
    throw runtime_error(string("Non-conforming parameters for function ")
		      + func->name());
  }
  return func->dim(parameter_dims);
}

static vector<unsigned int> const &
mkLengths(vector<Node const *> const &parameters) {
    vector<unsigned int> lengths(parameters.size());
    for (unsigned int j = 0; j < parameters.size(); ++j) {
        lengths[j] = parameters[j]->length();

    }
    return getUnique(lengths);
}

static vector<vector<unsigned int> > const &
mkParameterDims(vector<Node const *> const &parameters) {
    vector<vector<unsigned int> > dims(parameters.size());
    for (unsigned int j = 0; j < parameters.size(); ++j) {
        dims[j] = parameters[j]->dim();
    }
    return getUnique(dims);
}

LogicalNode::LogicalNode(Function const *function, 
			 vector<Node const *> const &parameters)
  : DeterministicNode(mkDim(function,parameters), parameters),
    _func(function),
    _parameters(nchain()),
    _dims(mkParameterDims(parameters)),
    _lengths(mkLengths(parameters))
{
    for (unsigned int n = 0; n < nchain(); ++n) {
	_parameters[n].reserve(parameters.size());
	for (unsigned long j = 0; j < parameters.size(); ++j) {
	    _parameters[n].push_back(parameters[j]->value(n));
	}
	
    }
}

LogicalNode::~LogicalNode()
{
}

string LogicalNode::deparse(vector<string> const &parents) const
{
    string name = "(";
    name.append(_func->deparse(parents));
    name.append(")");
	      
    return name;
}

void LogicalNode::deterministicSample(unsigned int chain)
{
  _func->evaluate(_data + chain * _length, _parameters[chain], _lengths, _dims);
}

bool LogicalNode::isLinear(GraphMarks const &linear_marks, bool fixed) const
{
    vector<bool> mask(parents().size());
    for (unsigned int i = 0; i < parents().size(); ++i) {
	Node const *p = parents()[i];
        if (linear_marks.graph().contains(p)) {
	    switch(linear_marks.mark(p)) {
            case MARK_NULL:
                mask[i] = false;
                break;
	    case MARK_TRUE:
		mask[i] = true;
		break;
	    case MARK_FALSE:
                //Parent is a non-linear function. No way to recover.
		return false;
		break;
	    default:
		throw logic_error("Invalid marks in LogicalNode::isLinear");
	    }
	}
        else {
            //We don't care if the function is non-linear in these parameters
            mask[i] = false; 
        }
    }

    vector<bool> fixed_mask;
    if (fixed) {
	for (unsigned int i = 0; i < parents().size(); ++i) {
	    fixed_mask.push_back(parents()[i]->isObserved());
	}
    }

    return _func->isLinear(mask, fixed_mask);
}

bool LogicalNode::isScale(GraphMarks const &scale_marks, bool fixed) const
{
    unsigned index = 0;
    bool have_index = false;
    vector<Node const *> const &par = parents();
    for (unsigned int i = 0; i < par.size(); ++i) {
	if (scale_marks.graph().contains(par[i])) {
	    switch(scale_marks.mark(par[i])) {
            case MARK_NULL:
		break;
	    case MARK_TRUE:
		if (!have_index) {
		    have_index = true;
		    index = i;
		}
		else {
		    return false;
		}
		break;
	    case MARK_FALSE:
		return false;
		break;
	    default:
		throw logic_error("Invalid marks in LogicalNode::isScale");
	    }
	}
    }
    if (!have_index) {
	return true;//FIXME: What is the best solution here?
    }
    
    vector<bool> fixed_mask;
    if (fixed) {
	for (unsigned int i = 0; i < par.size(); ++i) {
	    fixed_mask.push_back(par[i]->isObserved());
	}
    }

    return _func->isScale(index, fixed_mask);
}

//FIXME: Code overlap with isLinear
bool LogicalNode::isPower(GraphMarks const &linear_marks, bool fixed) const
{
    //Scale functions are, by default, fixed power functions
    if (isScale(linear_marks, false))
	return true;

    //Otherwise we check for a power function

    vector<bool> mask(parents().size());
    for (unsigned int i = 0; i < parents().size(); ++i) {
	Node const *p = parents()[i];
        if (linear_marks.graph().contains(p)) {
	    switch(linear_marks.mark(p)) {
            case MARK_NULL:
                mask[i] = false;
                break;
	    case MARK_TRUE:
		mask[i] = true;
		break;
	    case MARK_FALSE:
                //Parent is a non-power function. No way to recover.
		return false;
		break;
	    default:
		throw logic_error("Invalid marks in LogicalNode::isLinear");
	    }
	}
        else {
            //We don't care about these parameters
            mask[i] = false; 
        }
    }

    vector<bool> fixed_mask;
    if (fixed) {
	for (unsigned int i = 0; i < parents().size(); ++i) {
	    fixed_mask.push_back(parents()[i]->isObserved());
	}
    }

    return _func->isPower(mask, fixed_mask);
}

bool LogicalNode::checkParentValues(unsigned int chain) const
{
    return _func->checkParameterValue(_parameters[chain], _lengths, _dims);
}

Node *LogicalNode::clone(vector<Node const*> const &parents) const
{
    return new LogicalNode(_func, parents);
}

bool LogicalNode::isDiscreteValued() const
{
    vector<bool> mask(parents().size());
    for (unsigned long j = 0; j < parents().size(); ++j) {
        mask[j] = parents()[j]->isDiscreteValued();
    }
    return _func->isDiscreteValued(mask);
}
