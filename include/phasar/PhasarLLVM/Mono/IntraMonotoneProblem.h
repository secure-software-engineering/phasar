/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IntraMonotoneProblem.h
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef INTRAMONOTONEPROBLEM_H_
#define INTRAMONOTONEPROBLEM_H_

#include <phasar/Config/ContainerConfiguration.h>
#include <string>

namespace psr {

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
  virtual std::string DtoString(D d) = 0;
};

} // namespace psr

#endif
