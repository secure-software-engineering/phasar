/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_POINTSTOINFO_H
#define PHASAR_PHASARLLVM_POINTER_POINTSTOINFO_H

#include "phasar/PhasarLLVM/Pointer/PointsToInfoBase.h"

#include <type_traits>

namespace psr {

template <typename PTATraits,
          typename = std::enable_if_t<is_PointsToTraits_v<PTATraits>>>
class PointsToInfoRef {
public:
private:
  /// TODO: implement non-owning type erased PointsToInfo
};

template <typename PTATraits,
          typename = std::enable_if_t<is_PointsToTraits_v<PTATraits>>>
class PointsToInfo {
public:
private:
  /// TODO: implement owning type erased PointsToInfo based on PointsToInfoRef
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_POINTSTOINFO_H
