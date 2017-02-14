/*
 * KillAll.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_KILLALL_HH_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_KILLALL_HH_

#include <set>
#include <memory>
#include "../FlowFunction.hh"

using namespace std;

template<typename D>
class KillAll : public FlowFunction<D> {
private:
	KillAll() = default;
public:
	virtual ~KillAll() = default;
	KillAll(const KillAll& k) = delete;
	KillAll& operator= (const KillAll& k) = delete;
	set<D> computeTargets(D source) override { return set<D>(); }
	static shared_ptr<KillAll<D>> v()
	{
		static shared_ptr<KillAll> instance = shared_ptr<KillAll>(new KillAll);
		return instance;
	}
};

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_KILLALL_HH_ */
