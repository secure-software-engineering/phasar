#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <optional>
#include <utility>

namespace llvm {
class CallBase;
class Value;
class Type;
class Function;
} // namespace llvm

namespace psr {

/// Assuming that `CallSite` is a virtual call through a vtable, retrieves the
/// index in the vtable of the virtual function called.
[[nodiscard]] std::optional<unsigned>
getVFTIndex(const llvm::CallBase *CallSite);

/// Similar to getVFTIndex(), but also returns a pointer to the vtable
[[nodiscard]] std::optional<std::pair<const llvm::Value *, uint64_t>>
getVFTIndexAndVT(const llvm::CallBase *CallSite);

/// Detects the pattern \c call(load(GEP(base, const_indices...))) with a
/// typed (>=3-operand) GEP, i.e. an indirect call through a struct function
/// pointer field. Distinct from the 2-operand raw-pointer C++ vptr case
/// handled by \c getVFTIndexAndVT.
///
/// Returns \c {base_ptr, all_GEP_indices, gep_source_elem_ty} on match,
/// or \c std::nullopt otherwise.
[[nodiscard]] std::optional<std::tuple<
    const llvm::Value *, llvm::SmallVector<uint64_t, 3>, llvm::Type *>>
getStructVCallInfo(const llvm::CallBase *CallSite);

/// Checks whether the signature of `DestFun` matches the required withature of
/// `CallSite`, such that `DestFun` qualifies as callee-candidate, if `CallSite`
/// is an indirect/virtual call.
[[nodiscard]] bool isConsistentCall(const llvm::CallBase *CallSite,
                                    const llvm::Function *DestFun);
} // namespace psr
