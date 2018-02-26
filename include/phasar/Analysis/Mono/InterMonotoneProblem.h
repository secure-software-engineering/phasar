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

#include "../../config/ContainerConfiguration.h"
#include <string>
using namespace std;

template <typename N, typename D, typename M, typename C, typename I>
class InterMonotoneProblem {
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
  virtual MonoSet<D> returnFlow(N CallSite, M Callee, N RetStmt, N RetSite,
                                const MonoSet<D> &In) = 0;
  virtual MonoSet<D> callToRetFlow(N CallSite, N RetSite,
                                   const MonoSet<D> &In) = 0;
  virtual MonoMap<N, MonoSet<D>> initialSeeds() = 0;
  virtual string DtoString(D d) = 0;
  virtual string CtoString(C c) = 0;
};

#endif
