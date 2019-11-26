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

#include <initializer_list>
#include <set>
#include <string>
#include <type_traits>

#include <phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h>

namespace psr {

class ProjectIRDB;
template <typename T, typename M> class TypeHierarchy;
template <typename V> class PointsToInfo;
template <typename N, typename M> class ICFG;

template <typename N, typename D, typename M, typename T, typename V, typename I>
class InterMonoProblem : public IntraMonoProblem<N, D, M, T, V, I> {
  static_assert(std::is_base_of_v<ICFG<N, M>, I>,
                "I must implement the ICFG interface!");

protected:
  const I *ICF;

public:
  InterMonoProblem(const ProjectIRDB *IRDB, const TypeHierarchy<T, M> *TH,
                   const I *ICF, const PointsToInfo<V> *PT,
                   std::set<std::string> EntryPoints = {})
      : IntraMonoProblem<N, D, M, T, V, I>(IRDB, TH, ICF, PT, EntryPoints), ICF(ICF) {
  }

  InterMonoProblem(const InterMonoProblem &copy) = delete;
  InterMonoProblem(InterMonoProblem &&move) = delete;
  InterMonoProblem &operator=(const InterMonoProblem &copy) = delete;
  InterMonoProblem &operator=(InterMonoProblem &&move) = delete;

  virtual MonoSet<D> callFlow(N CallSite, M Callee, const MonoSet<D> &In) = 0;
  virtual MonoSet<D> returnFlow(N CallSite, M Callee, N ExitStmt, N RetSite,
                                const MonoSet<D> &In) = 0;
  virtual MonoSet<D> callToRetFlow(N CallSite, N RetSite, MonoSet<M> Callees,
                                   const MonoSet<D> &In) = 0;
};

} // namespace psr

#endif
