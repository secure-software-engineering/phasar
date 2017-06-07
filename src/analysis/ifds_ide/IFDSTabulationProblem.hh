/*
 * IFDSTabulationProblem.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_IFDSTABULATIONPROBLEM_HH_
#define ANALYSIS_IFDS_IDE_IFDSTABULATIONPROBLEM_HH_

#include <type_traits>
#include <map>
#include <set>
#include "../icfg/ICFG.hh"
#include "SolverConfiguration.hh"
#include "FlowFunctions.hh"

using namespace std;

template<typename N, typename D, typename M, typename I>
class IFDSTabulationProblem : public FlowFunctions<N,D,M> {
public:
	SolverConfiguration solver_config;
	virtual ~IFDSTabulationProblem() = default;
	virtual I interproceduralCFG() = 0;
	virtual map<N, set<D>> initialSeeds() = 0;
	virtual D zeroValue() = 0;
};

#endif /* ANALYSIS_IFDS_IDE_IFDSTABULATIONPROBLEM_HH_ */
