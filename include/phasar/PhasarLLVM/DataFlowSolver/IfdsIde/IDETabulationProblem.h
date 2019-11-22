/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IDETabulationProblem.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_IDETABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_IFDSIDE_IDETABULATIONPROBLEM_H_

#include <memory>
#include <set>
#include <string>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/JoinLattice.h>

namespace psr {

class ProjectIRDB;
class TypeHierarchy;
class PointsToInfo;

template <typename N, typename D, typename M, typename V, typename I>
class IDETabulationProblem : public IFDSTabulationProblem<N, D, M, I>,
                             public virtual EdgeFunctions<N, D, M, V>,
                             public virtual JoinLattice<V>,
                             public virtual ValuePrinter<V> {
  static_assert(std::is_base_of_v<ICFG<N, M>, I>,
                "I must implement the ICFG interface!");

public:
  IDETabulationProblem(const ProjectIRDB *IRDB, const TypeHierarchy *TH,
                       const I *ICF, const PointsToInfo *PT,
                       std::set<std::string> EntryPoints = {})
      : IFDSTabulationProblem<N, D, M, I>(IRDB, TH, ICF, PT, EntryPoints) {}
  ~IDETabulationProblem() override = default;
  virtual std::shared_ptr<EdgeFunction<V>> allTopFunction() = 0;
  virtual void printIDEReport(std::ostream &os, SolverResults<N, D, V> &SR) {
    os << "No IDE report available!\n";
  }
};
} // namespace psr

#endif
