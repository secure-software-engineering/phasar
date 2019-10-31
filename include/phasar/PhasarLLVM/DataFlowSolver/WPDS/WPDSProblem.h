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

#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/WPDS/WPDSOptions.h>

namespace psr {

template <typename N, typename D, typename M, typename V, typename I>
class WPDSProblem : public virtual IDETabulationProblem<N, D, M, V, I> {
public:
  ~WPDSProblem() override = default;
  virtual SearchDirection getSearchDirection() = 0;
  virtual WPDSType getWPDSTy() = 0;
  virtual bool recordWitnesses() = 0;
};

} // namespace psr

#endif
