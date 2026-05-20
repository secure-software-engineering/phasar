#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/Casting.h"

#include <concepts>
#include <unordered_map>

namespace psr {

/// Memoised walker for global-variable pointer initializers.
///
/// Traverses a \c llvm::Constant initializer and collects the \c ValueId of
/// every pointer-typed sub-constant it contains (direct pointer, GEP base,
/// or pointer elements of an aggregate).  Results are cached so shared
/// sub-expressions are not revisited.
///
/// Create one instance per analysis run; it is tied to a single
/// \c ValueCompressor via the \p GetVar callback.
struct GlobalInitCache {
  std::unordered_map<const llvm::Constant *, llvm::SmallVector<ValueId, 1>>
      Cache;

  /// Returns the \c ValueId slice for all pointer-typed constants reachable
  /// from \p Const.  \p GetVar maps an \c llvm::Value* to a \c ValueId
  /// (typically \c getOrInsertVar).
  template <std::invocable<const llvm::Value *> GetVarFn>
  [[nodiscard]] llvm::ArrayRef<ValueId>
  getOrCreate(const llvm::Constant *Const, GetVarFn &&GetVar) {
    if (definitelyContainsNoPointer(Const)) {
      return {};
    }

    auto [It, Inserted] = Cache.try_emplace(Const);
    if (!Inserted) {
      return It->second;
    }
    auto &Vec = It->second;

    if (llvm::isa<llvm::ConstantPointerNull>(Const)) {
      return {};
    }

    if (const auto *CGep = llvm::dyn_cast<llvm::GEPOperator>(Const)) {
      // TODO: Properly handle constant GEPs
      return getOrCreate(
          llvm::cast<llvm::Constant>(CGep->getPointerOperand()), GetVar);
    }

    if (Const->getType()->isPointerTy()) {
      Vec.push_back(std::invoke(GetVar, Const));
      return Vec;
    }

    // TODO: Get rid of the recursion

    if (const auto *Agg = llvm::dyn_cast<llvm::ConstantAggregate>(Const)) {
      if (Agg->getType()->isArrayTy() &&
          definitelyContainsNoPointer(
              Agg->getType()->getArrayElementType())) {
        return {};
      }
      for (size_t I = 0, N = Agg->getNumOperands(); I < N; ++I) {
        const auto *Elem = llvm::cast<llvm::Constant>(
            Agg->getAggregateElement(I)->stripPointerCastsAndAliases());
        auto Sub = getOrCreate(Elem, GetVar);
        Vec.append(Sub.begin(), Sub.end());
      }
    }

    // TODO: more

    return Vec;
  }
};

} // namespace psr
