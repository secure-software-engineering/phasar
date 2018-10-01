/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSTabulationProblem.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_IFDSTABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_IFDSIDE_IFDSTABULATIONPROBLEM_H_

#include <map>
#include <set>
#include <string>

#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/SolverResults.h>
#include <phasar/PhasarLLVM/IfdsIde/SolverConfiguration.h>
#include <phasar/PhasarLLVM/Utils/Printer.h>

namespace psr {

template <typename N, typename D, typename M, typename I>
class IFDSTabulationProblem : public FlowFunctions<N, D, M>,
                              public NodePrinter<N>,
                              public DataFlowFactPrinter<D>,
                              public MethodPrinter<M> {
public:
  SolverConfiguration solver_config;
  virtual ~IFDSTabulationProblem() = default;
  virtual I interproceduralCFG() = 0;
  virtual std::map<N, std::set<D>> initialSeeds() = 0;
  virtual D zeroValue() = 0;
  virtual bool isZeroValue(D d) const = 0;
  void setSolverConfiguration(SolverConfiguration conf) {
    solver_config = conf;
  }
  SolverConfiguration getSolverConfiguration() { return solver_config; }
  virtual void printIFDSReport(std::ostream &os,
                               SolverResults<N, D, BinaryDomain> &SR) {
    os << "No IFDS report available!";
  }
};
} // namespace psr

#endif
