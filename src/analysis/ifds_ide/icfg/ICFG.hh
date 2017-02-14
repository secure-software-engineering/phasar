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

using namespace std;

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

	virtual bool isCallStmt(N stmt) = 0;

	virtual bool isExitStmt(N stmt) = 0;

	virtual bool isStartPoint(N stmt) = 0;

	virtual set<N> allNonCallStartNodes() = 0;

	virtual bool isFallThroughSuccessor(N stmt, N succ) = 0;

	virtual bool isBranchTarget(N stmt, N succ) = 0;
};

#endif /* ANALYSIS_ICFG_HH_ */
