/*
 * InterproceduralCFG.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_INTERPROCEDURALCFG_HH_
#define ANALYSIS_IFDS_IDE_INTERPROCEDURALCFG_HH_

#include <vector>
#include <set>

using namespace std;

template<class N, class M>
class InterproceduralCFG {
public:
	virtual ~InterproceduralCFG() = default;

	virtual M getMethodOf(const N& n) = 0;

	virtual vector<N> getPredsOf(const N& u) = 0;

	virtual vector<N> getSuccsOf(const N& n) = 0;

	virtual multiset<M> getCalleesOfCallAt(const N& n) = 0;

	virtual multiset<N> getCallersOf(const M& m) = 0;

	virtual set<N> getCallsFromWithin(const M& m) = 0;

	virtual multiset<N> getStartPointsOf(const M& m) = 0;

	virtual multiset<N> getReturnSitesOfCallAt(const N& n) = 0;

	virtual bool isCallStmt(const N& stmt) = 0;

	virtual bool isExitStmt(const N& stmt) = 0;

	virtual bool isStartPoint(const N& stmt) = 0;

	virtual set<N> allNonCallStartNodes() = 0;

	virtual bool isFallTroughSuccessor(const N& stmt, const N& succ) = 0;

	virtual bool isBranchTarget(const N& stmt, const N& succ) = 0;
};

#endif /* ANALYSIS_IFDS_IDE_INTERPROCEDURALCFG_HH_ */
