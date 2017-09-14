/*
 * IFDSTabulationProblem.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_IFDSTABULATIONPROBLEM_HH_
#define ANALYSIS_IFDS_IDE_IFDSTABULATIONPROBLEM_HH_

#include "../icfg/ICFG.hh"
#include "FlowFunctions.hh"
#include "SolverConfiguration.hh"
#include <map>
#include <set>
#include <string>
#include <type_traits>

using namespace std;

template <typename N, typename D, typename M, typename I>
class IFDSTabulationProblem : public FlowFunctions<N, D, M> {
public:
  SolverConfiguration solver_config;
  virtual ~IFDSTabulationProblem() = default;
  virtual I interproceduralCFG() = 0;
  virtual map<N, set<D>> initialSeeds() = 0;
  virtual D zeroValue() = 0;
	virtual string D_to_string(D d) = 0;
};

#endif /* ANALYSIS_IFDS_IDE_IFDSTABULATIONPROBLEM_HH_ */
