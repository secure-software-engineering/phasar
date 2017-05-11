/*
 * Identity.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOW_FUNC_IDENTITY_HH_
#define ANALYSIS_IFDS_IDE_FLOW_FUNC_IDENTITY_HH_

#include <set>
#include <memory>
#include "../FlowFunction.hh"

using namespace std;

template<typename D>
class Identity : public FlowFunction<D> {
private:
	Identity() = default;
public:
	virtual ~Identity() = default;
	Identity(const Identity& i) = delete;
	Identity& operator= (const Identity& i) = delete;
	// simply return what the user provides
	set<D> computeTargets(D source) override { return { source }; }
	static shared_ptr<Identity> v()
	{
		static shared_ptr<Identity> instance = shared_ptr<Identity>(new Identity);
		return instance;
	}
};

#endif /* ANALYSIS_IFDS_IDE_FLOW_FUNC_IDENTITY_HH_ */
