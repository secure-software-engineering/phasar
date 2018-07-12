/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * InterMonotoneProblem.h
 *
 *  Created on: 23.06.2017
 *      Author: philipp
 */

#pragma once

#include <string>
#include <utility>

#include <phasar/Config/ContainerConfiguration.h>

#include <phasar/Utils/Macros.h>
#include <phasar/PhasarLLVM/Mono/Values/ValueBase.h>
#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>

namespace psr {

template <typename N, typename D, typename M, typename I>
class InterMonotoneProblem {
public:
  using Node_t      = N;
  using Value_t     = D;
  using Method_t    = M;
  using ICFG_t      = I;

private:
  template <typename T1, typename T2>
  void InterMonotoneProblem_check() {
    static_assert(std::is_base_of<psr::ICFG<N,M>, I>::value, "Template class I must be a sub class of ICFG<N, M>\n");
  }

protected:
  I& ICFG;

public:
  InterMonotoneProblem(I& Icfg) : ICFG(Icfg) {}

  InterMonotoneProblem(const InterMonotoneProblem &copy) = delete;
  InterMonotoneProblem(InterMonotoneProblem &&move) = delete;
  InterMonotoneProblem& operator=(const InterMonotoneProblem &copy) = delete;
  InterMonotoneProblem& operator=(InterMonotoneProblem &&move) = delete;


  virtual ~InterMonotoneProblem() = default;
  I& getICFG() noexcept { return ICFG; }
  virtual MonoSet<D> join(const MonoSet<D> &Lhs, const MonoSet<D> &Rhs) = 0;
  virtual bool sqSubSetEqual(const MonoSet<D> &Lhs, const MonoSet<D> &Rhs) = 0;
  virtual MonoSet<D> normalFlow(N Stmt, const MonoSet<D> &In) = 0;
  virtual MonoSet<D> callFlow(N CallSite, M Callee, const MonoSet<D> &In) = 0;
  virtual MonoSet<D> returnFlow(N CallSite, M Callee, N RetSite,
                                const MonoSet<D> &In) = 0;
  virtual MonoSet<D> callToRetFlow(N CallSite, N RetSite,
                                   const MonoSet<D> &In) = 0;
  virtual MonoMap<N, MonoSet<D>> initialSeeds() = 0;
  virtual std::string DtoString(D d) = 0;
  virtual bool recompute(M Callee) = 0;
};

} // namespace psr
