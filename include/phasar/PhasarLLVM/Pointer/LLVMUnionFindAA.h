#pragma once

#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

namespace psr {
extern template class CallingContextSensUnionFindAA<LLVMPAGDomain>;
extern template class IndirectionSensUnionFindAA<LLVMPAGDomain>;

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
  LLVMLocalUnionFindAliasIterator(AAResT &&AARes,
                                  const ValueCompressor<PAGVariable> *VC)
      : detail::LLVMLocalUnionFindAliasIteratorBase(psr::assertNotNull(VC)),
        AARes(PSR_FWD(AARes)), VC(VC) {}

  void foreachAliasOf(ValueId VId,
                      std::invocable<const llvm::Value *> auto WithAlias,
                      const llvm::Function *Context = nullptr) {
    const auto AliasHandler = [this,
                               WithAlias = copyOrRef(WithAlias)](ValueId VId) {
      for (auto V : VC->id2vars(VId)) {
        if (const auto *LLVMVar = V.valueOrNull()) [[likely]] {
          std::invoke(WithAlias, LLVMVar);
        }
      }
    };

    auto &&RawVars = AARes.getRawAliasSet(VId);
    if (Context) {
      auto Vars = PSR_FWD(RawVars);
      Vars &= getOrDefault(GlobalsOrInFun, Context);
      Vars.foreach (AliasHandler);
    } else {
      RawVars.foreach (AliasHandler);
    }
  }

  void foreachAliasOf(const llvm::Value *Val,
                      std::invocable<const llvm::Value *> auto WithAlias,
                      const llvm::Function *Context) {
    if (auto ValId = VC->getOrNull(Val)) {
      foreachAliasOf(*ValId, copyOrRef(WithAlias), Context);
    }
  }
  void foreachAliasOf(const llvm::Value *Val,
                      std::invocable<const llvm::Value *> auto WithAlias) {
    foreachAliasOf(Val, copyOrRef(WithAlias), psr::getFunction(Val));
  }

private:
  AAResT AARes;
  const ValueCompressor<PAGVariable> *VC{};
};

} // namespace psr
