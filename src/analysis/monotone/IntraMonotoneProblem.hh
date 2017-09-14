/*
 * IntraMonotoneProblem.hh
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef INTRAMONOTONEPROBLEM_HH_
#define INTRAMONOTONEPROBLEM_HH_

#include "../../utils/ContainerConfiguration.hh"
#include <string>
using namespace std;

template <typename N, typename D, typename M, typename C>
class IntraMonotoneProblem {
protected:
  C CFG;
  M Function;

public:
  IntraMonotoneProblem(C Cfg, M F) : CFG(Cfg), Function(F) {}
  virtual ~IntraMonotoneProblem() = default;
  C getCFG() { return CFG; }
  M getFunction() { return Function; }
  virtual MonoSet<D> join(const MonoSet<D> &Lhs, const MonoSet<D> &Rhs) = 0;
  virtual bool sqSubSetEqual(const MonoSet<D> &Lhs, const MonoSet<D> &Rhs) = 0;
  virtual MonoSet<D> flow(N S, const MonoSet<D> &In) = 0;
  virtual MonoMap<N, MonoSet<D>> initialSeeds() = 0;
  virtual string D_to_string(D d) = 0;
};

#endif
