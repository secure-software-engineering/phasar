/*
 * Transfer.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_TRANSFER_HH_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_TRANSFER_HH_

#include <set>
#include "../FlowFunction.hh"

using namespace std;

template<typename D>
class Transfer : public FlowFunction<D> {
private:
	D toValue;
	D fromValue;

public:
	Transfer(D toValue, D fromValue) : toValue(toValue), fromValue(fromValue) { }
	virtual ~Transfer() = default;
	set<D> computeTargets(D source) override
	{
		if (source == fromValue)
			return { source, toValue };
		else if (source == toValue)
			return { };
		else
			return { source };
	}
};

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_TRANSFER_HH_ */
