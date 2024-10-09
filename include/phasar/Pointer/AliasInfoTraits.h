/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_POINTER_ALIASINFOTRAITS_H
#define PHASAR_POINTER_ALIASINFOTRAITS_H

#include "phasar/Utils/BoxedPointer.h"
#include "phasar/Utils/MaybeUniquePtr.h"

#include "llvm/ADT/DenseSet.h"

namespace psr {

template <typename T> struct AliasInfoTraits {
  //   using n_t;
  //   using v_t;
  //   using AliasSetTy;
  //   using AliasSetPtrTy;
  //   using AllocationSiteSetPtrTy;
};

template <typename V, typename N> struct DefaultAATraits {
  using n_t = N;
  using v_t = V;
  using AliasSetTy = llvm::DenseSet<v_t>;
  using AliasSetPtrTy = BoxedConstPtr<AliasSetTy>;
  using AllocationSiteSetPtrTy = MaybeUniquePtr<const AliasSetTy>;
};
} // namespace psr

#endif // PHASAR_POINTER_ALIASINFOTRAITS_H
