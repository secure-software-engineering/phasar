/*
 * Kill.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_KILL_HH_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_KILL_HH_

#include <set>
#include "../FlowFunction.hh"

using namespace std;

template<typename D>
class Kill : public FlowFunction<D> {
private:
	D killValue;

public:
	Kill(D killValue) : killValue(killValue) { }
	virtual ~Kill() = default;
	set<D> computeTargets(D source) override
	{
		if (source == killValue)
			return { };
		else
			return { source };
	}
};

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_KILL_HH_ */
