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
#include <unordered_map>

#include <phasar/PhasarLLVM/Utils/Printer.h>
#include <phasar/Utils/BitVectorSet.h>

namespace psr {

template <typename N, typename D, typename M, typename C>
class IntraMonoProblem : public NodePrinter<N>,
                         public DataFlowFactPrinter<D>,
                         public MethodPrinter<M> {
protected:
  C CFG;
  M Function;
  IntraMonoProblem(C Cfg) : CFG(Cfg) {}

public:
  IntraMonoProblem(C Cfg, M F) : CFG(Cfg), Function(F) {}
  ~IntraMonoProblem() override = default;
  C getCFG() { return CFG; }
  M getFunction() { return Function; }
  virtual BitVectorSet<D> join(const BitVectorSet<D> &Lhs,
                               const BitVectorSet<D> &Rhs) = 0;
  virtual bool sqSubSetEqual(const BitVectorSet<D> &Lhs,
                             const BitVectorSet<D> &Rhs) = 0;
  virtual BitVectorSet<D> normalFlow(N S, const BitVectorSet<D> &In) = 0;
  virtual std::unordered_map<N, BitVectorSet<D>> initialSeeds() = 0;
};

} // namespace psr

#endif
