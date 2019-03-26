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

private:
  template <typename T1, typename T2> void InterMonoProblem_check() {
    static_assert(std::is_base_of<psr::ICFG<N, M>, std::remove_reference_t<I>>::value,
                  "Template class I must be a sub class of ICFG<N, M>\n");
  }

protected:
  I ICFG;

public:
  InterMonoProblem(I Icfg) : ICFG(Icfg) {}

  InterMonoProblem(const InterMonoProblem &copy) = delete;
  InterMonoProblem(InterMonoProblem &&move) = delete;
  InterMonoProblem &operator=(const InterMonoProblem &copy) = delete;
  InterMonoProblem &operator=(InterMonoProblem &&move) = delete;

  virtual ~InterMonoProblem() = default;
  I getICFG() noexcept { return ICFG; }
  virtual D join(const D &Lhs, const D &Rhs) = 0;
  virtual bool sqSubSetEqual(const D &Lhs, const D &Rhs) = 0;
  virtual D normalFlow(const N Stmt, const D &In) = 0;
  virtual D callFlow(const N CallSite, const M Callee,
                            const D &In) = 0;
  virtual D returnFlow(const N CallSite, const M Callee,
                              const N RetSite, const D &In) = 0;
  virtual D callToRetFlow(const N CallSite, const N RetSite,
                                 const D &In) = 0;
  virtual MonoMap<N, D> initialSeeds() = 0;
  virtual bool recompute(const M Callee) = 0;
};

} // namespace psr

#endif
