#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/SCCId.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/DenseSet.h"

namespace psr {

enum class FunctionId : uint32_t;

template <typename G> struct UsedGlobalsHolder {
  using GlobalSet = llvm::SmallDenseSet<G>;

  TypedVector<SCCId<FunctionId>, GlobalSet> GlobsPerSCC;
  TypedVector<SCCId<FunctionId>, GlobalSet> InitialGlobsPerSCC;
};

} // namespace psr
