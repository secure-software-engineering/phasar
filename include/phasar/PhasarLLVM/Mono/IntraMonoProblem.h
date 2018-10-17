/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IntraMonoProblem.h
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_INTRAMONOPROBLEM_H_
#define PHASAR_PHASARLLVM_MONO_INTRAMONOPROBLEM_H_

#include <string>

#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Utils/Printer.h>

namespace psr {

template <typename N, typename D, typename M, typename C>
class IntraMonoProblem : public NodePrinter<N>,
                         public DataFlowFactPrinter<D>,
                         public MethodPrinter<M> {
protected:
  C CFG;
  M Function;

public:
  IntraMonoProblem(C Cfg, M F) : CFG(Cfg), Function(F) {}
  virtual ~IntraMonoProblem() = default;
  C getCFG() { return CFG; }
  M getFunction() { return Function; }
  virtual MonoSet<D> join(const MonoSet<D> &Lhs, const MonoSet<D> &Rhs) = 0;
  virtual bool sqSubSetEqual(const MonoSet<D> &Lhs, const MonoSet<D> &Rhs) = 0;
  virtual MonoSet<D> flow(N S, const MonoSet<D> &In) = 0;
  virtual MonoMap<N, MonoSet<D>> initialSeeds() = 0;
};

} // namespace psr

#endif
