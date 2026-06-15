#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/HashUtils.h"
#include "phasar/Utils/UsedGlobalsHolder.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/SmallVector.h"

#include <unordered_map>

namespace llvm {
class Function;
class Value;
class GlobalVariable;
class Instruction;
} // namespace llvm

namespace psr::monoifds {

class AliasCache {
public:
  static constexpr llvm::StringLiteral LogCategory = "monoifds::AliasCache";

  // Passed AI should already be FilteredAliasSet or similar
  explicit AliasCache(
      LLVMAliasIteratorRef AI [[clang::lifetime_capture_by(this)]],
      llvm::function_ref<bool(const llvm::Value *)> SkipSeedsCallBack
      [[clang::lifetime_capture_by(this)]],
      const UsedGlobalsHolder<const llvm::GlobalVariable *>::GlobalSet
          *PermittedGlobals [[clang::lifetime_capture_by(this)]],
      std::pmr::memory_resource *MRes [[clang::lifetime_capture_by(this)]])
      : AI(AI), SkipSeedsCallBack(SkipSeedsCallBack),
        PermittedGlobals(&assertNotNull(PermittedGlobals)),
        Cache(&assertNotNull(MRes)) {}

  [[nodiscard]] llvm::ArrayRef<const llvm::Value *>
  getAliasSet(const llvm::Value *Fact, const llvm::Instruction *At);

private:
  // NOTE: Used the node_hash_map from
  // [parallel-hash-map](https://github.com/greg7mdp/parallel-hashmap) here
  // for the paper-eval!
  using node_hash_map = std::pmr::unordered_map<
      std::pair<const llvm::Function *, const llvm::Value *>,
      llvm::SmallVector<const llvm::Value *>, PairHash>;

  LLVMAliasIteratorRef AI;
  llvm::function_ref<bool(const llvm::Value *)> SkipSeedsCallBack;
  const UsedGlobalsHolder<const llvm::GlobalVariable *>::GlobalSet
      *PermittedGlobals{};
  node_hash_map Cache;
};
} // namespace psr::monoifds
