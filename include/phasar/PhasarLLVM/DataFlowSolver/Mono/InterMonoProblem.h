/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * InterMonoProblem.h
 *
 *  Created on: 23.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_INTERMONOPROBLEM_H_
#define PHASAR_PHASARLLVM_MONO_INTERMONOPROBLEM_H_

#include <set>

#include <phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h>
#include <phasar/Utils/BitVectorSet.h>

namespace psr {

class ProjectIRDB;
template <typename T, typename F> class TypeHierarchy;
template <typename V, typename N> class PointsToInfo;
template <typename N, typename F> class ICFG;

template <typename N, typename D, typename F, typename T, typename V,
          typename I>
class InterMonoProblem : public IntraMonoProblem<N, D, F, T, V, I> {
  static_assert(std::is_base_of_v<ICFG<N, F>, I>,
                "I must implement the ICFG interface!");

protected:
  const I *ICF;

public:
  InterMonoProblem(const ProjectIRDB *IRDB, const TypeHierarchy<T, F> *TH,
                   const I *ICF, const PointsToInfo<V, N> *PT,
                   std::set<std::string> EntryPoints = {})
      : IntraMonoProblem<N, D, F, T, V, I>(IRDB, TH, ICF, PT, EntryPoints),
        ICF(ICF) {}

  InterMonoProblem(const InterMonoProblem &copy) = delete;
  InterMonoProblem(InterMonoProblem &&move) = delete;
  InterMonoProblem &operator=(const InterMonoProblem &copy) = delete;
  InterMonoProblem &operator=(InterMonoProblem &&move) = delete;

  virtual BitVectorSet<D> callFlow(N CallSite, F Callee,
                                   const BitVectorSet<D> &In) = 0;
  virtual BitVectorSet<D> returnFlow(N CallSite, F Callee, N ExitStmt,
                                     N RetSite, const BitVectorSet<D> &In) = 0;
  virtual BitVectorSet<D> callToRetFlow(N CallSite, N RetSite,
                                        std::set<F> Callees,
                                        const BitVectorSet<D> &In) = 0;

  const I *getICFG() const { return ICF; }
};

} // namespace psr

#endif
