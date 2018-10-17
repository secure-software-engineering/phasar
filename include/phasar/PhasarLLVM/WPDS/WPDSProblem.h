/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_WPDSPROBLEM_H_
#define PHASAR_PHASARLLVM_WPDS_WPDSPROBLEM_H_

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <type_traits>

namespace psr {

template <typename N, typename D, typename M, typename V, typename I>
class WPDSProblem {
private:
  I ICFG;

  void WPDSProblem_check() {
    static_assert(std::is_base_of<psr::ICFG<N, M>, I>::value,
                  "Template class I must be a sub class of ICFG<N, M>\n");
  }

public:
  WPDSProblem(I ICFG) : ICFG(ICFG) {}
  virtual ~WPDSProblem() = default;
  virtual void solve() {}
};

} // namespace psr

#endif
