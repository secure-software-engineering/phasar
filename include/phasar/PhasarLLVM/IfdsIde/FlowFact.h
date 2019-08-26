/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFACT_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFACT_H_

#include <iosfwd>

namespace psr {

class FlowFact {
public:
  virtual ~FlowFact() = default;
  virtual void print(std::ostream &OS) const = 0;
  virtual bool equal_to(const FlowFact &FF) const = 0;
  virtual bool less(const FlowFact &FF) const = 0;
};

static inline std::ostream &operator<<(std::ostream &OS, const FlowFact &F) {
  F.print(OS);
  return OS;
}

static inline bool operator==(const FlowFact &F, const FlowFact &G) {
  return F.equal_to(G);
}

static inline bool operator!=(const FlowFact &F, const FlowFact &G) {
  return !F.equal_to(G);
}

static inline bool operator<(const FlowFact &F, const FlowFact &G) {
  return F.less(G);
}

} // namespace psr

#endif
