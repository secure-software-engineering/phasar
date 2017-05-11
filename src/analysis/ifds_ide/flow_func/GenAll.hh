/*
 * Gen.hh
 *
 *  Created on: 05.05.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_GENALL_HH_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_GENALL_HH_

#include <set>
#include "../FlowFunction.hh"

using namespace std;

template<typename D>
class GenAll : public FlowFunction<D> {
private:
	set<D> genValues;
	D zeroValue;

public:
	GenAll(set<D> genValue, D zeroValue) : genValues(genValues), zeroValue(zeroValue) { }
	virtual ~GenAll() = default;
	set<D> computeTargets(D source) override
	{
		if (source == zeroValue) {
			genValues.insert(source);
			return genValues;
		} else {
			return { source };
		}
	}
};

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_GEN_HH_ */
