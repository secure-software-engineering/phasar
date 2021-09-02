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
    Owner.reserve(InitialCapacity);
    OwnedPTS.reserve(InitialCapacity);
  }

  PointsToSetTy *acquire() {
    Owner.push_back(std::make_unique<PointsToSetTy>());
    OwnedPTS[Owner.back().get()] = Owner.size() - 1;
    return Owner.back().get();
  }
  void release(PointsToSetTy *PTS) {
    if (Owner.empty()) {
      return;
    }

    assert(Owner.size() == OwnedPTS.size());

    if (auto It = OwnedPTS.find(PTS); It != OwnedPTS.end()) {
      auto Idx = It->second;

      OwnedPTS[Owner.back().get()] = Idx;
      OwnedPTS.erase(PTS);

      std::swap(Owner[Idx], Owner.back());
      Owner.pop_back();
    }
  }

private:
  std::vector<std::unique_ptr<PointsToSetTy>> Owner;
  llvm::DenseMap<PointsToSetTy *, size_t> OwnedPTS;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_POINTSTOSETOWNER_H_