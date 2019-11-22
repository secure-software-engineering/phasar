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

#include <phasar/PhasarLLVM/Mono/IntraMonoProblem.h>
#include <phasar/Utils/BitVectorSet.h>

namespace psr {

template <typename N, typename D, typename M, typename I>
class InterMonoProblem : public IntraMonoProblem<N, D, M, I> {

private:
  using IntraMonoProblem<N, D, M, I>::getCFG;
  using IntraMonoProblem<N, D, M, I>::getFunction;

protected:
  I ICFG;

public:
  InterMonoProblem(I Icfg) : IntraMonoProblem<N, D, M, I>(Icfg), ICFG(Icfg) {}

  InterMonoProblem(const InterMonoProblem &copy) = delete;
  InterMonoProblem(InterMonoProblem &&move) = delete;
  InterMonoProblem &operator=(const InterMonoProblem &copy) = delete;
  InterMonoProblem &operator=(InterMonoProblem &&move) = delete;

  I getICFG() noexcept { return ICFG; }

  virtual BitVectorSet<D> callFlow(N CallSite, M Callee,
                                   const BitVectorSet<D> &In) = 0;
  virtual BitVectorSet<D> returnFlow(N CallSite, M Callee, N ExitStmt,
                                     N RetSite, const BitVectorSet<D> &In) = 0;
  virtual BitVectorSet<D> callToRetFlow(N CallSite, N RetSite,
                                        std::set<M> Callees,
                                        const BitVectorSet<D> &In) = 0;
};

} // namespace psr

#endif
