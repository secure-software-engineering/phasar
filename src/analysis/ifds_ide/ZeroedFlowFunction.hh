/*
 * ZeroedFlowFunctions.hh
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_ZEROEDFLOWFUNCTION_HH_
#define ANALYSIS_IFDS_IDE_ZEROEDFLOWFUNCTION_HH_

#include <set>
#include "FlowFunction.hh"
using namespace std;

template<class D>
class ZeroedFlowFunction : public FlowFunction<D> {
private:
	shared_ptr<FlowFunction<D>> delegate;
	D zerovalue;
public:
	ZeroedFlowFunction(shared_ptr<FlowFunction<D>> ff, D zv) : delegate(ff), zerovalue(zv) {}
	set<D> computeTargets(D source) override
	{
		if (source == zerovalue) {
			set<D> result = delegate->computeTargets(source);
			result.insert(zerovalue);
			return result;
		} else {
			return delegate->computeTargets(source);
		}
	}
};

#endif /* ANALYSIS_IFDS_IDE_ZEROEDFLOWFUNCTION_HH_ */
