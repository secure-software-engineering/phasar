/*
 * ICFG.hh
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_ICFG_HH_
#define ANALYSIS_IFDS_IDE_ICFG_HH_

#include <vector>
#include <set>
#include <iostream>

using namespace std;

enum class CallType {
	none = 0,
	normal = 1,
	summary = 2,
	special_summary = 3,
	unavailable = 4
};

ostream& operator<<(ostream& os, const CallType& CT);

template<typename N, typename M>
class ICFG {
public:
	virtual ~ICFG() = default;

	virtual M getMethodOf(N n) = 0;

	virtual vector<N> getPredsOf(N u) = 0;

	virtual vector<N> getSuccsOf(N n) = 0;

	virtual set<M> getCalleesOfCallAt(N n) = 0;

	virtual set<N> getCallersOf(M m) = 0;

	virtual set<N> getCallsFromWithin(M m) = 0;

	virtual set<N> getStartPointsOf(M m) = 0;

	virtual set<N> getReturnSitesOfCallAt(N n) = 0;

	/**
	 * We return an int rather than a boolean value, since we would also like to
	 * distinguish between different categories of functions that are called.
	 * A class that inherits from the ICFG interface can define a suitable
	 * enumeration that represents various kinds of categories. Different
	 * categories can than be treated different within the solver (e.g. special
	 * summaries may be used, etc.). IMPORTANT: by convention returning 0
	 * indicates a non-call statement, returning 1 indicates a usual function call
	 * that should be treated as a usual function by the solver. Other values are
	 * free to represent function categories that should receive special treatment
	 * by the solver.
	 */
	virtual CallType isCallStmt(N stmt) = 0;

	virtual bool isExitStmt(N stmt) = 0;

	virtual bool isStartPoint(N stmt) = 0;

	virtual set<N> allNonCallStartNodes() = 0;

	virtual bool isFallThroughSuccessor(N stmt, N succ) = 0;

	virtual bool isBranchTarget(N stmt, N succ) = 0;
};

#endif /* ANALYSIS_ICFG_HH_ */
