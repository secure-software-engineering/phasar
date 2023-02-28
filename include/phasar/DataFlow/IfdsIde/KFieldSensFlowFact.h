/******************************************************************************
 * Copyright (c) 2023 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_KFIELDSENSFLOWFACT_H
#define PHASAR_DATAFLOW_IFDSIDE_KFIELDSENSFLOWFACT_H

#include "llvm/ADT/SmallVector.h"

#include <optional>

template <typename d_t, unsigned K = 3, unsigned OffsetLimit = 1024>
class KFieldSensFlowFact {
  using offset_int_t = int16_t;
  static_assert(
      std::numeric_limits<offset_int_t>::max() > OffsetLimit &&
      "offset_int_t needs to be able to hold an offset of OffsetLimit");

public:
  KFieldSensFlowFact() = default;
  ~KFieldSensFlowFact() = default;

  KFieldSensFlowFact(d_t BaseValue) : BaseValue(BaseValue) {}

  KFieldSensFlowFact getStored() {
    auto Result = *this;
    if (Result.AccessPath.size() == K) {
      for (unsigned I = 0; I < K - 1; I++) {
        Result.AccessPath[I] = Result.AccessPath[I + 1];
      }
      if constexpr (K > 0) {
        Result.AccessPath[K - 1] = 0;
      }
      Result.FollowedByAny = true;
      return Result;
    }
    Result.AccessPath.push_back(0);
    return Result;
  }

  std::optional<KFieldSensFlowFact> getLoaded(uint64_t TargetTypeSize,
                                              int64_t FollowedOffset = 0) {
    auto Result = *this;
    if (Result.AccessPath.size() > 0) {
      if (Result.AccessPath.back() == OffsetLimit ||
          std::abs(Result.AccessPath.back()) <
              static_cast<int64_t>(TargetTypeSize) + FollowedOffset) {
        Result.AccessPath.pop_back();
      } else {
        return std::nullopt;
      }
    } else if (!Result.FollowedByAny) {
      return std::nullopt;
    }
    return Result;
  }

  // Increment the offset of the first indirection by Offset
  KFieldSensFlowFact getWithOffset(int64_t Offset) {
    auto Result = *this;
    int64_t NewFirstOffset64 =
        static_cast<int64_t>(Result.AccessPath.back()) + Offset;
    constexpr auto UpcastOffsetLimit = static_cast<int64_t>(OffsetLimit);
    offset_int_t NewFirstOffset;
    if (NewFirstOffset64 >= UpcastOffsetLimit ||
        NewFirstOffset64 <= -UpcastOffsetLimit) {
      NewFirstOffset = OffsetLimit;
    } else {
      NewFirstOffset = static_cast<offset_int_t>(NewFirstOffset64);
    }
    Result.AccessPath.back() = NewFirstOffset;
    return Result;
  }

  KFieldSensFlowFact getFirstOverapproximated() {
    auto Result = *this;
    Result.AccessPath.back() = OffsetLimit;
    return Result;
  }

  void print(llvm::raw_ostream &OS) const {
    OS << *BaseValue;
    unsigned I = AccessPath.size();
    while (I > 0) {
      I--;
      OS << ".(" << AccessPath[I] << ')';
    }
    if (FollowedByAny) {
      OS << ".*";
    }
  }

  const d_t getBaseValue() const { return BaseValue; }

protected:
  d_t BaseValue;
  llvm::SmallVector<offset_int_t,
                    std::min<unsigned>(K, 8 / sizeof(offset_int_t))>
      AccessPath;
  bool FollowedByAny = false;
};

#endif /* PHASAR_DATAFLOW_IFDSIDE_KFIELDSENSFLOWFACT_H */
