/*
 * Union.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_UNION_HH_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_UNION_HH_

#include <set>
#include <vector>
#include "../FlowFunction.hh"

using namespace std;

template<typename D>
class Union : public FlowFunction<D> {
private:
	const vector<FlowFunction<D>> funcs;

public:
	Union(const vector<FlowFunction<D>>& funcs) : funcs(funcs) { }
	virtual ~Union() = default;
	set<D> computeTargets(const D& source) override
	{
		set<D> result;
		for (const FlowFunction<D>& func : funcs) {
			set<D> target = func.computeTarget(source);
			result.insert(target.begin(), target.end());
		}
		return result;
	}
	static FlowFunction<D> setunion(const vector<FlowFunction<D>>& funcs)
	{
		vector<FlowFunction<D>> vec;
		for (const FlowFunction<D>& func : funcs)
			if (func != Identity<D>::v())
				vec.add(func);
		if (vec.size() == 1)
			return vec[0];
		else if (vec.empty())
			return Identity<D>::v();
		return Union(vec);
	}
};

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_UNION_HH_ */
