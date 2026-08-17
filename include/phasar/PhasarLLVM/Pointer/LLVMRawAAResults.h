#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Pointer/RawAAResult.h"
#include "phasar/Utils/NonNullPtr.h"

#include "llvm/IR/Instructions.h"

namespace psr {

/// Returns a \c ValueId handler suitable for \c RawAliasSet::foreach() that
/// maps each alias \c ValueId back to all of its underlying \c llvm::Value*
/// (via \p VC), forwarding non-null values to \p Callback.
constexpr std::invocable<ValueId> auto
llvmRawAliasHandler(const ValueCompressor<PAGVariable> &VC,
                    std::invocable<const llvm::Value *> auto Callback) {
  return [&VC, Callback{copyOrRef(Callback)}](ValueId Alias) {
    for (auto V : VC.id2vars(Alias)) {
      if (const auto *LLVMVar = V.valueOrNull()) [[likely]] {
        std::invoke(Callback, LLVMVar);
      }
    }
  };
}

/// CRTP mixin that adds the LLVM alias-iterator interface to a class that
/// holds a \c RawAAResult and a \c ValueCompressor<PAGVariable>.
///
/// Provides \c forallAliasesOf(), \c mayAlias(), and \c alias() overloads
/// accepting both \c llvm::Value* and \c ValueId arguments.  Results are
/// reported as \c llvm::Value* via the stored \c ValueCompressor.
///
/// The derived class must expose a \c VC member (pointer to a
/// \c ValueCompressor<PAGVariable>).
///
/// \tparam Derived The CRTP derived class.
/// \tparam AAResT  A type satisfying \c RawAAResult.
template <typename Derived, typename AAResT>
  requires RawAAResult<std::remove_cvref_t<AAResT>>
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct LLVMRawAliasIteratorMixin {
  [[no_unique_address]] AAResT AARes;

  using v_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;

  [[nodiscard]] decltype(auto) getRawAliasSet(ValueId ValId) const {
    return AARes.getRawAliasSet(ValId);
  }

  [[nodiscard]] const auto &base() const noexcept { return AARes; }

  void
  forallAliasesOf(ValueId VId, const auto & /*Inst*/,
                  std::invocable<const llvm::Value *> auto Callback) const {
    const auto &RawAliases = AARes.getRawAliasSet(VId);
    RawAliases.foreach (llvmRawAliasHandler(*self().VC, copyOrRef(Callback)));
  }

  void
  forallAliasesOf(const llvm::Value *Ptr, const auto &Inst,
                  std::invocable<const llvm::Value *> auto Callback) const {
    if (auto ValId = self().VC->getOrNull(Ptr)) {
      forallAliasesOf(*ValId, Inst, copyOrRef(Callback));
    }
  }

  [[nodiscard]] bool mayAlias(ValueId Ptr1, ValueId Ptr2) const {
    return AARes.mayAlias(Ptr1, Ptr2);
  }

  [[nodiscard]] bool mayAlias(ValueId Ptr1, ValueId Ptr2,
                              const auto & /*AtInstruction*/) const {
    return AARes.mayAlias(Ptr1, Ptr2);
  }

  [[nodiscard]] bool mayAlias(const llvm::Value *Ptr1,
                              const llvm::Value *Ptr2) const {
    auto ValId1 = self().VC->getOrNull(Ptr1);
    auto ValId2 = self().VC->getOrNull(Ptr2);

    return ValId1 && ValId2 && mayAlias(*ValId1, *ValId2);
  }

  [[nodiscard]] bool mayAlias(const llvm::Value *Ptr1, const llvm::Value *Ptr2,
                              const auto & /*AtInstruction*/) const {
    return mayAlias(Ptr1, Ptr2);
  }

  [[nodiscard]] AliasResult alias(const llvm::Value *Ptr1,
                                  const llvm::Value *Ptr2,
                                  const auto &AtInstruction) const {
    auto ValId1 = self().VC->getOrNull(Ptr1);
    auto ValId2 = self().VC->getOrNull(Ptr2);
    if (!ValId1 || !ValId2) {
      return AliasResult::NoAlias;
    }
    if (*ValId1 == *ValId2) {
      if (Ptr1 == Ptr2) {
        return AliasResult::MustAlias;
      }
      return !llvm::isa<llvm::LoadInst>(Ptr1) &&
                     !llvm::isa<llvm::LoadInst>(Ptr2)
                 ? AliasResult::MustAlias
                 : AliasResult::MayAlias;
    }
    return mayAlias(*ValId1, *ValId2, AtInstruction) ? AliasResult::MayAlias
                                                     : AliasResult::NoAlias;
  }

  [[nodiscard]] constexpr const Derived &self() const noexcept {
    return *static_cast<const Derived *>(this);
  }
};

template <typename AAResT>
  requires RawAAResult<std::remove_cvref_t<AAResT>>
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct LLVMRawAliasIterator
    : public LLVMRawAliasIteratorMixin<LLVMRawAliasIterator<AAResT>, AAResT> {
  MaybeUniquePtr<const ValueCompressor<PAGVariable>> VC;

  constexpr LLVMRawAliasIterator(
      AAResT &&AARes,
      MaybeUniquePtr<const ValueCompressor<PAGVariable>> VC PSR_LIFETIMEBOUND)
      : psr::LLVMRawAliasIteratorMixin<LLVMRawAliasIterator<AAResT>,
                                       AAResT>{PSR_FWD(AARes)},
        VC(std::move(VC)) {}
};

template <typename AAResT>
LLVMRawAliasIterator(AAResT, MaybeUniquePtr<const ValueCompressor<PAGVariable>>)
    -> LLVMRawAliasIterator<AAResT>;
template <typename AAResT>
LLVMRawAliasIterator(AAResT, const ValueCompressor<PAGVariable> *)
    -> LLVMRawAliasIterator<AAResT>;

namespace detail {
class LLVMLocalRawAliasIteratorBase {
public:
  explicit LLVMLocalRawAliasIteratorBase(
      const ValueCompressor<PAGVariable> &VC);

protected:
  llvm::DenseMap<const llvm::Function *, RawAliasSet<ValueId>> GlobalsOrInFun;
};
} // namespace detail

/// CRTP mixin adding a function-local view on top of a global
/// \c RawAAResult.
///
/// Extends \c LLVMRawAliasIteratorMixin with \c forallAliasesOf()
/// overloads that accept an \c llvm::Function* or \c llvm::Instruction*
/// context.  When a non-null context is provided, alias sets are intersected
/// with the set of variables that are visible in that function (globals plus
/// locals defined in that function), giving a function-local result even
/// though the underlying analysis is interprocedural.
template <typename Derived, typename AAResT>
class LLVMLocalRawAliasIteratorMixin
    : public detail::LLVMLocalRawAliasIteratorBase {
public:
  LLVMLocalRawAliasIteratorMixin(AAResT &&AARes,
                                 const ValueCompressor<PAGVariable> &VC)
      : detail::LLVMLocalRawAliasIteratorBase(VC), AARes(PSR_FWD(AARes)) {}

  [[nodiscard]] decltype(auto) getRawAliasSet(ValueId ValId) const {
    return AARes.getRawAliasSet(ValId);
  }

  [[nodiscard]] auto getRawAliasSet(ValueId ValId,
                                    const llvm::Function *Context) const {
    auto Vars = AARes.getRawAliasSet(ValId);
    if (Context) {
      Vars &= getOrDefault(GlobalsOrInFun, Context);
    }
    return Vars;
  }

  [[nodiscard]] const auto &base() const noexcept { return AARes; }

  [[nodiscard]] auto getRawAliasSet(ValueId ValId,
                                    const llvm::Instruction *Context) const {
    return getRawAliasSet(ValId, getFunction(Context));
  }

  void forallAliasesOf(ValueId VId, const llvm::Function *Context,
                       std::invocable<const llvm::Value *> auto WithAlias) {
    const auto AliasHandler =
        llvmRawAliasHandler(*self().VC, copyOrRef(WithAlias));

    auto &&RawVars = AARes.getRawAliasSet(VId);
    if (Context) {
      auto Vars = PSR_FWD(RawVars);
      Vars &= getOrDefault(GlobalsOrInFun, Context);
      Vars.foreach (AliasHandler);
    } else {
      RawVars.foreach (AliasHandler);
    }
  }

  void forallAliasesOf(const llvm::Value *Val, const llvm::Function *Context,
                       std::invocable<const llvm::Value *> auto WithAlias) {
    if (auto ValId = self().VC->getOrNull(Val)) {
      forallAliasesOf(*ValId, Context, copyOrRef(WithAlias));
    }
  }

  void forallAliasesOf(ValueId ValId, const llvm::Instruction *AtInstruction,
                       std::invocable<const llvm::Value *> auto WithAlias) {
    forallAliasesOf(ValId, psr::getFunction(AtInstruction),
                    copyOrRef(WithAlias));
  }

  void forallAliasesOf(const llvm::Value *Val,
                       const llvm::Instruction *AtInstruction,
                       std::invocable<const llvm::Value *> auto WithAlias) {
    forallAliasesOf(Val, psr::getFunction(AtInstruction), copyOrRef(WithAlias));
  }

  void forallAliasesOf(const llvm::Value *Val,
                       std::invocable<const llvm::Value *> auto WithAlias) {
    forallAliasesOf(Val, psr::getFunction(Val), copyOrRef(WithAlias));
  }

  [[nodiscard]] bool
  mayAlias(ValueId ValId1, ValueId ValId2,
           const llvm::Instruction * /*AtInstruction*/ = nullptr) const {
    // XXX: Should we filter by AtInstruction-context here as well?
    return AARes.mayAlias(ValId1, ValId2);
  }

  [[nodiscard]] bool
  mayAlias(const llvm::Value *Ptr1, const llvm::Value *Ptr2,
           const llvm::Instruction * /*AtInstruction*/ = nullptr) const {
    auto ValId1 = self().VC->getOrNull(Ptr1);
    auto ValId2 = self().VC->getOrNull(Ptr2);

    // XXX: Should we filter by AtInstruction-context here as well?
    return ValId1 && ValId2 && AARes.mayAlias(*ValId1, *ValId2);
  }

  [[nodiscard]] AliasResult alias(const llvm::Value *Ptr1,
                                  const llvm::Value *Ptr2,
                                  const auto &AtInstruction) const {
    auto ValId1 = self().VC->getOrNull(Ptr1);
    auto ValId2 = self().VC->getOrNull(Ptr2);
    if (!ValId1 || !ValId2) {
      return AliasResult::NoAlias;
    }
    if (*ValId1 == *ValId2) {
      if (Ptr1 == Ptr2) {
        return AliasResult::MustAlias;
      }
      return !llvm::isa<llvm::LoadInst>(Ptr1) &&
                     !llvm::isa<llvm::LoadInst>(Ptr2)
                 ? AliasResult::MustAlias
                 : AliasResult::MayAlias;
    }
    return mayAlias(*ValId1, *ValId2, AtInstruction) ? AliasResult::MayAlias
                                                     : AliasResult::NoAlias;
  }

  [[nodiscard]] constexpr const Derived &self() const noexcept {
    return *static_cast<const Derived *>(this);
  }

private:
  AAResT AARes;
};

template <typename AAResT>
class LLVMLocalRawAliasIterator
    : public LLVMLocalRawAliasIteratorMixin<LLVMLocalRawAliasIterator<AAResT>,
                                            AAResT> {
  friend LLVMLocalRawAliasIteratorMixin<LLVMLocalRawAliasIterator<AAResT>,
                                        AAResT>;

public:
  LLVMLocalRawAliasIterator(AAResT &&AARes,
                            NonNullPtr<const ValueCompressor<PAGVariable>> VC
                                PSR_LIFETIMEBOUND)
      : LLVMLocalRawAliasIteratorMixin<LLVMLocalRawAliasIterator<AAResT>,
                                       AAResT>(PSR_FWD(AARes), *VC),
        VC(VC) {}

private:
  NonNullPtr<const ValueCompressor<PAGVariable>> VC;
};
} // namespace psr
