/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFACT_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFACT_H_

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {

/// A common superclass of edge-facts used by non-template IDETabulationProblems
class EdgeFact {
public:
  virtual ~EdgeFact() = default;
  virtual void print(llvm::raw_ostream &OS) const = 0;
};

static inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                            const EdgeFact &E) {
  E.print(OS);
  return OS;
}

} // namespace psr

#endif
