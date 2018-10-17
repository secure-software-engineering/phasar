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

#include <string>
#include <type_traits>

#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Utils/Printer.h>

namespace psr {

template <typename N, typename D, typename M, typename I>
class InterMonoProblem : public NodePrinter<N>,
                         public DataFlowFactPrinter<D>,
                         public MethodPrinter<M> {
public:
  using Node_t = N;
  using Domain_t = D;
  using Method_t = M;
  using ICFG_t = std::remove_reference_t<I>;

private:
  template <typename T1, typename T2> void InterMonoProblem_check() {
    static_assert(std::is_base_of<psr::ICFG<Node_t, Method_t>, ICFG_t>::value,
                  "Template class I must be a sub class of ICFG<N, M>\n");
  }

protected:
  ICFG_t &ICFG;

public:
  InterMonoProblem(ICFG_t &Icfg) : ICFG(Icfg) {}

  InterMonoProblem(const InterMonoProblem &copy) = delete;
  InterMonoProblem(InterMonoProblem &&move) = delete;
  InterMonoProblem &operator=(const InterMonoProblem &copy) = delete;
  InterMonoProblem &operator=(InterMonoProblem &&move) = delete;

  virtual ~InterMonoProblem() = default;
  ICFG_t &getICFG() noexcept { return ICFG; }
  virtual Domain_t join(const Domain_t &Lhs, const Domain_t &Rhs) = 0;
  virtual bool sqSubSetEqual(const Domain_t &Lhs, const Domain_t &Rhs) = 0;
  virtual Domain_t normalFlow(const Node_t Stmt, const Domain_t &In) = 0;
  virtual Domain_t callFlow(const Node_t CallSite, const Method_t Callee,
                            const Domain_t &In) = 0;
  virtual Domain_t returnFlow(const Node_t CallSite, const Method_t Callee,
                              const Node_t RetSite, const Domain_t &In) = 0;
  virtual Domain_t callToRetFlow(const Node_t CallSite, const Node_t RetSite,
                                 const Domain_t &In) = 0;
  virtual MonoMap<Node_t, Domain_t> initialSeeds() = 0;
  virtual bool recompute(const Method_t Callee) = 0;
};

} // namespace psr

#endif
