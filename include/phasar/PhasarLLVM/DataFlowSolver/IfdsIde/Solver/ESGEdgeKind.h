/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_ESGEDGEKIND_H
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_ESGEDGEKIND_H

namespace psr {
enum class ESGEdgeKind { Normal, Call, CallToRet, SkipUnknownFn, Ret, Summary };

constexpr bool isInterProc(ESGEdgeKind Kind) noexcept {
  return Kind == ESGEdgeKind::Call || Kind == ESGEdgeKind::Ret;
}

} // namespace psr

#endif // PHASAR_PHASARLLVM_IFDSIDE_SOLVER_ESGEDGEKIND_H
