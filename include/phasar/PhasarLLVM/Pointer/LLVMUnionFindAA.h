#pragma once

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

namespace psr {
extern template class CallingContextSensUnionFindAA<LLVMPAGDomain>;
extern template class IndirectionSensUnionFindAA<LLVMPAGDomain>;

inline constexpr std::invocable<ValueId> auto
llvmUnionFindAliasHandler(const ValueCompressor<PAGVariable> &VC,
                          std::invocable<const llvm::Value *> auto Callback) {
  return [&VC, Callback](ValueId Alias) {
    for (auto V : VC.id2vars(Alias)) {
      if (const auto *LLVMVar = V.valueOrNull()) [[likely]] {
        std::invoke(Callback, LLVMVar);
      }
    }
  };
}

namespace pag {
/// Utility class to make pag::PBMixin<IndirectionSensUnionFindAA,
/// LLVMCGProvider> implement PBStrategy.
class LLVMCGProvider : public LLVMPAGDomain {
public:
  constexpr LLVMCGProvider(NonNullPtr<const LLVMBasedCallGraph> CG) noexcept
      : CG(CG) {}

  void withCalleesOfCallAt(n_t Inst,
                           std::invocable<f_t> auto WithCallee) const {
    for (const auto *Callee : CG->getCalleesOfCallAt(Inst)) {
      std::invoke(WithCallee, Callee);
    }
  }

private:
  NonNullPtr<const LLVMBasedCallGraph> CG;
};
} // namespace pag

template <typename AAResT, typename Var2IdMapper, typename Id2VarMapper>
  requires UnionFindAAResult<std::remove_cvref_t<AAResT>>
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct LLVMUnionFindAliasIterator {
  AAResT AARes;
  NonNullPtr<const ValueCompressor<PAGVariable>> VC;

  void forallAliasesOf(ValueId VId, const auto & /*Inst*/,
                       std::invocable<const llvm::Value *> auto Callback) {
    const auto &RawAliases = AARes.getRawAliasSet(VId);
    RawAliases.foreach (llvmUnionFindAliasHandler(VC, copyOrRef(Callback)));
  }

  void forallAliasesOf(const llvm::Value *Ptr, const auto &Inst,
                       std::invocable<const llvm::Value *> auto Callback) {
    if (auto ValId = VC->getOrNull(Ptr)) {
      foreachAliasOf(*ValId, Inst, copyOrRef(Callback));
    }
  }
};

namespace detail {
class LLVMLocalUnionFindAliasIteratorBase {
public:
  explicit LLVMLocalUnionFindAliasIteratorBase(
      const ValueCompressor<PAGVariable> &VC);

protected:
  llvm::DenseMap<const llvm::Function *, RawAliasSet<ValueId>> GlobalsOrInFun;
};
} // namespace detail

template <typename AAResT>
class LLVMLocalUnionFindAliasIterator
    : public detail::LLVMLocalUnionFindAliasIteratorBase {
public:
  LLVMLocalUnionFindAliasIterator(
      AAResT &&AARes, NonNullPtr<const ValueCompressor<PAGVariable>> VC)
      : detail::LLVMLocalUnionFindAliasIteratorBase(*VC), AARes(PSR_FWD(AARes)),
        VC(VC) {}

  void foreachAliasOf(ValueId VId, const llvm::Function *Context,
                      std::invocable<const llvm::Value *> auto WithAlias) {
    const auto AliasHandler =
        llvmUnionFindAliasHandler(VC, copyOrRef(WithAlias));

    auto &&RawVars = AARes.getRawAliasSet(VId);
    if (Context) {
      auto Vars = PSR_FWD(RawVars);
      Vars &= getOrDefault(GlobalsOrInFun, Context);
      Vars.foreach (AliasHandler);
    } else {
      RawVars.foreach (AliasHandler);
    }
  }

  void foreachAliasOf(const llvm::Value *Val, const llvm::Function *Context,
                      std::invocable<const llvm::Value *> auto WithAlias) {
    if (auto ValId = VC->getOrNull(Val)) {
      foreachAliasOf(*ValId, Context, copyOrRef(WithAlias));
    }
  }

  void foreachAliasOf(const llvm::Value *Val,
                      const llvm::Instruction *AtInstruction,
                      std::invocable<const llvm::Value *> auto WithAlias) {
    foreachAliasOf(Val, psr::getFunction(AtInstruction), copyOrRef(WithAlias));
  }

  void foreachAliasOf(const llvm::Value *Val,
                      std::invocable<const llvm::Value *> auto WithAlias) {
    foreachAliasOf(Val, psr::getFunction(Val), copyOrRef(WithAlias));
  }

private:
  AAResT AARes;
  NonNullPtr<const ValueCompressor<PAGVariable>> VC;
};

} // namespace psr
