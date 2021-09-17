/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_POINTSTOSETOWNER_H_
#define PHASAR_PHASARLLVM_POINTER_POINTSTOSETOWNER_H_

#include "llvm/ADT/DenseMap.h"
#include <memory>
#include <vector>

namespace psr {
template <typename PointsToSetTy> class PointsToSetOwner {
public:
  explicit PointsToSetOwner() = default;
  explicit PointsToSetOwner(size_t InitialCapacity) {
    OwnedPTS.reserve(InitialCapacity);
  }

  PointsToSetTy *acquire() {
    auto Ptr = std::make_unique<PointsToSetTy>();
    auto Ret = Ptr.get();
    OwnedPTS.try_emplace(Ret, std::move(Ptr));
    return Ret;
  }
  void release(PointsToSetTy *PTS) { OwnedPTS.erase(PTS); }

private:
  llvm::DenseMap<PointsToSetTy *, std::unique_ptr<PointsToSetTy>> OwnedPTS;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_POINTSTOSETOWNER_H_