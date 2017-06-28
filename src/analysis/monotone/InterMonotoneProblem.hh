/*
 * InterMonotoneProblem.hh
 *
 *  Created on: 23.06.2017
 *      Author: philipp
 */

#ifndef INTERMONOTONEPROBLEM_HH_
#define INTERMONOTONEPROBLEM_HH_

#include "../../utils/ContainerConfiguration.hh"
using namespace std;

template <typename N, typename D, typename M, typename I>
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
};

#endif
