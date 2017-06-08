/*
 * CFG.hh
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_ICFG_CFG_HH_
#define SRC_ANALYSIS_ICFG_CFG_HH_

#include <iostream>
#include <vector>
#include <set>
#include <utility>
using namespace std;

template <typename M, typename N>
class CFG {
public:
	virtual ~CFG() = default;

	virtual vector<N> getPredsOf(N u) = 0;

	virtual vector<N> getSuccsOf(N n) = 0;

	virtual vector<pair<N,N>> getAllControlFlowEdges() = 0;

	virtual vector<N> getAllInstructions() = 0;

	virtual bool isCallStmt(N stmt) = 0;

	virtual bool isExitStmt(N stmt) = 0;

	virtual bool isStartPoint(N stmt) = 0;

	virtual vector<N> allNonCallStartNodes() = 0;

	virtual bool isFallThroughSuccessor(N stmt, N succ) = 0;

	virtual bool isBranchTarget(N stmt, N succ) = 0;
};

#endif /* SRC_ANALYSIS_ICFG_CFG_HH_ */
