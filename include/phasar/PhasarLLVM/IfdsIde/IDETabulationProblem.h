/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IDETabulationProblem.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_IDETABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_IFDSIDE_IDETABULATIONPROBLEM_H_

#include <memory>
#include <string>

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/JoinLattice.h>

namespace psr {

template <typename N, typename D, typename M, typename V, typename I>
class IDETabulationProblem : public virtual IFDSTabulationProblem<N, D, M, I>,
                             public virtual EdgeFunctions<N, D, M, V>,
                             public virtual JoinLattice<V>,
                             public virtual ValuePrinter<V> {
public:
  ~IDETabulationProblem() override = default;
  virtual std::shared_ptr<EdgeFunction<V>> allTopFunction() = 0;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
  virtual void emitTextReport(std::ostream &os, SolverResults<N, D, V> SR) {
    os << "No text report available!\n";
  }
#pragma clang diagnostic pop
private:
  using IFDSTabulationProblem<N, D, M, I>::emitTextReport;
};
} // namespace psr

#endif
