/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFACT_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFACT_H_

#include <iosfwd>

namespace psr {

/// A common superclass of edge-facts used by non-template IDETabulationProblems
/// (for example in plugins)
class EdgeFact {
public:
  virtual ~EdgeFact() = default;
  virtual void print(std::ostream &os) const = 0;
};

static inline std::ostream &operator<<(std::ostream &OS, const EdgeFact &E) {
  E.print(OS);
  return OS;
}

} // namespace psr

#endif
