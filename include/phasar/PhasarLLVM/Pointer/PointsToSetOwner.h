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

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

#include "llvm/ADT/DenseMap.h"

namespace psr {
template <typename PointsToSetTy> class PointsToSetOwner {
public:
  explicit PointsToSetOwner() noexcept = default;

  PointsToSetTy *acquire() {
    auto Ptr = std::make_unique<PointsToSetTy>();
    auto Ret = Ptr.get();
    OwnedPTS.try_emplace(Ret, std::move(Ptr));
    return Ret;
  }
  void release(PointsToSetTy *PTS) noexcept { OwnedPTS.erase(PTS); }

  void reserve(size_t Capacity) { OwnedPTS.reserve(Capacity); }

  template <typename CallBackTy,
            typename = std::enable_if_t<
                std::is_invocable_v<CallBackTy, PointsToSetTy *>>>
  void forEachPointsToSet(CallBackTy &&CallBack) {
    for (auto &[PTS, Unused] : OwnedPTS) {
      std::invoke(std::forward<CallBackTy>(CallBack), PTS);
    }
  }

  template <typename CallBackTy,
            typename = std::enable_if_t<
                std::is_invocable_v<CallBackTy, const PointsToSetTy *>>>
  void forEachPointsToSet(CallBackTy &&CallBack) const {
    for (auto &[PTS, Unused] : OwnedPTS) {
      std::invoke(std::forward<CallBackTy>(CallBack),
                  static_cast<const PointsToSetTy *>(PTS));
    }
  }

private:
  /// Note: Cannot use a set here, because llvm::DenseSet requires the key-type
  /// to be copy-constructible and the STL containers do not support
  /// heterogenous lookup as of C++17
  llvm::DenseMap<PointsToSetTy *, std::unique_ptr<PointsToSetTy>> OwnedPTS;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_POINTSTOSETOWNER_H_