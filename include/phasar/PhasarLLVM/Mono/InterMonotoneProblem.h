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

#ifndef INTERMONOTONEPROBLEM_H_
#define INTERMONOTONEPROBLEM_H_

#include <string>
#include <phasar/Config/ContainerConfiguration.h>

#include <phasar/Utils/Macros.h>
#include <phasar/PhasarLLVM/Mono/Values/ValueBase.h>
using namespace std;

namespace psr {

template <typename N, typename D, typename M, typename C, typename I>
class InterMonotoneProblem {
private:
  template <typename T1, typename T2>
  void InterMonotoneProblem_check() {
    static_assert(std::is_base_of<ValueBase<T1, T2, D>, D>::value, "Template class D must be a sub class of ValueBase<T1, T2, D> with T1, T2 templates\n");
  }

protected:
  I ICFG;

public:
  InterMonotoneProblem(I Icfg) : ICFG(Icfg) {}
  virtual ~InterMonotoneProblem() = default;
  I getICFG() { return ICFG; }
  virtual MonoSet<D> join(const MonoSet<D> &Lhs, const MonoSet<D> &Rhs) = 0;
  virtual bool sqSubSetEqual(const MonoSet<D> &Lhs, const MonoSet<D> &Rhs) = 0;
  virtual MonoSet<D> normalFlow(N Stmt, const MonoSet<D> &In) = 0;
  virtual MonoSet<D> callFlow(N CallSite, M Callee, const MonoSet<D> &In) = 0;
  virtual MonoSet<D> callFlow(N CallSite, M Callee, const MonoSet<D> &In, const MonoSet<D> &KnownOut) = 0;
  virtual MonoSet<D> returnFlow(N CallSite, M Callee, N RetStmt, N RetSite,
                                const MonoSet<D> &In) = 0;
  virtual MonoSet<D> returnFlow(N CallSite, M Callee, N RetStmt, N RetSite,
                                const MonoSet<D> &In,
                                const MonoSet<D> &KnownOut) = 0;
  virtual MonoSet<D> callToRetFlow(N CallSite, N RetSite,
                                   const MonoSet<D> &In) = 0;
  virtual MonoMap<N, MonoSet<D>> initialSeeds() = 0;
  virtual string DtoString(D d) = 0;
  virtual string CtoString(C c) = 0;
};

} // namespace psr

#endif
