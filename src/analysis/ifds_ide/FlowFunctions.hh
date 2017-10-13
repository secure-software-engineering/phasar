/*
 * FlowFunctions.hh
 *
 *  Created on: 03.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_FLOWFUNCTIONS_HH_
#define ANALYSIS_FLOWFUNCTIONS_HH_

#include <memory>
#include <vector>
#include "FlowFunction.hh"

using namespace std;

template<typename N, typename D, typename M>
class FlowFunctions {
public:
	virtual ~FlowFunctions() = default;
	virtual shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr, N succ) = 0;
	virtual shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt, M destMthd) = 0;
	virtual shared_ptr<FlowFunction<D>> getRetFlowFunction(N callSite, M calleeMthd, N exitStmt, N retSite) = 0;
	virtual shared_ptr<FlowFunction<D>> getCallToRetFlowFunction(N callSite, N retSite) = 0;
	virtual shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N curr, M destMthd) = 0;
};

#endif /* ANALYSIS_ABSTRACTFLOWFUNCTIONS_HH_ */
