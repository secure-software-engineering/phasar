/*
 * IDETabulationProblemWZeroedFF.hh
 *
 *  Created on: 14.12.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_IDETABULATIONPROBLEMWZEROEDFF_HH_
#define ANALYSIS_IFDS_IDE_IDETABULATIONPROBLEMWZEROEDFF_HH_

#include <memory>
#include "IDETabluationProblem.hh"
#include "ZeroedFlowFunction.hh"
using namespace std;

/*
 * Provide a helper class, that follows the delegation pattern to automatically add the zero value for all four flow functions.
 * The four flow functions are modified in such a way that they add the zero value to their result set.
 * All other functions remain unaffected and can be called through this delegation class.
 */
template<typename N, typename D, typename M, typename V, typename I>
class IDETabulationProblemWAutoZeroedFF : public IDETabluationProblem<N,D,M,V,I> {
private:
	IDETabluationProblem<N,D,M,V,I>& problem;

public:
	explicit IDETabulationProblemWAutoZeroedFF(IDETabluationProblem<N,D,M,V,I>&& ideproblem) : problem(ideproblem) {}

	// Delegate all flow functions to be zeroed flow functions

	shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr, N succ) override
	{
		cout << "called zeroed getNormalFlowFunction()" << endl;
		return make_shared<ZeroedFlowFunction<D>>(problem.getNormalFlowFunction(curr, succ), problem.zeroValue());
	}

	shared_ptr<FlowFunction<D>> getCallFlowFuntion(N callStmt, M destinationMethod) override
	{
		cout << "called zeroed getCallFlowFunction()" << endl;
		return make_shared<ZeroedFlowFunction<D>>(problem.getCallFlowFuntion(callStmt, destinationMethod), problem.zeroValue());
	}

	shared_ptr<FlowFunction<D>> getRetFlowFunction(N callSite, M calleeMethod, N exitStmt, N returnStmt) override
	{
		cout << "called zeroed getRetFlowFunction()" << endl;
		return make_shared<ZeroedFlowFunction<D>>(problem.getRetFlowFunction(callSite, calleeMethod, exitStmt, returnStmt), problem.zeroValue());
	}

	shared_ptr<FlowFunction<D>> getCallToRetFlowFunction(N callSite, N returnSite) override
	{
		cout << "called zeroed getCallToRetFlowFunction()" << endl;
		return make_shared<ZeroedFlowFunction<D>>(problem.getCallToRetFlowFunction(callSite, returnSite), problem.zeroValue());
	}

	// Delegate the other functions to be unmodified

	I interproceduralCFG() override
	{
		return problem.interproceduralCFG();
	}

	map<N, set<D>> initialSeeds() override
	{
		return problem.initialSeeds();
	}

	D zeroValue() override
	{
		return problem.zeroValue();
	}

	V topElement() override
	{
		return problem.topElement();
	}

	V bottomElement() override
	{
		return problem.bottomElement();
	}

	V join(V left, V right) override
	{
		return problem.join(left, right);
	}

	shared_ptr<EdgeFunction<V>> allTopFunction() override
	{
		return problem.allTopFunction();
	}


	bool followReturnsPastSeeds() override
	{
		return problem.followReturnsPastSeeds();
	}

	bool autoAddZero() override
	{
		return problem.autoAddZero();
	}

	bool computeValues() override
	{
		return problem.computeValues();
	}

	bool recordEdges() override
	{
		return problem.recordEdges();
	}

	shared_ptr<EdgeFunction<V>> getNormalEdgeFunction(N src,D srcNode,N tgt,D tgtNode) override
	{
		return problem.getNormalEdgeFunction(src, srcNode, tgt, tgtNode);
	}

	shared_ptr<EdgeFunction<V>> getCallEdgeFunction(N callStmt,D srcNode,M destinationMethod,D destNode) override
	{
		return problem.getCallEdgeFunction(callStmt, srcNode, destinationMethod, destNode);
	}

	shared_ptr<EdgeFunction<V>> getReturnEdgeFunction(N callSite, M calleeMethod,N exitStmt,D exitNode,N returnSite,D retNode) override
	{
		return problem.getReturnEdgeFunction(callSite, calleeMethod, exitStmt, exitNode, returnSite, retNode);
	}

	shared_ptr<EdgeFunction<V>> getCallToReturnEdgeFunction(N callStmt,D callNode,N returnSite,D returnSideNode) override
	{
		return problem.getCallToReturnEdgeFunction(callStmt, callNode, returnSite, returnSideNode);
	}

};


#endif /* ANALYSIS_IFDS_IDE_IDETABULATIONPROBLEMWZEROEDFF_HH_ */
