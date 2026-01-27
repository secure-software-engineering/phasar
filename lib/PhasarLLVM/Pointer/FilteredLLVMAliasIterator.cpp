#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasIterator.h"

#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/IR/Constants.h"

#include "FilteredAliasesUtils.h"

using namespace psr;

static void
forallAliasesOfImpl(LLVMAliasIteratorRef Underlying, const llvm::Value *V,
                    const llvm::Instruction *At, const llvm::Function *Fun,
                    llvm::function_ref<void(const llvm::Value *)> WithAlias) {
  if (!Fun) {
    Underlying.forallAliasesOf(V, At, WithAlias);
    return;
  }

  const auto *Base = V->stripPointerCastsAndAliases();

  Underlying.forallAliasesOf(V, At, [=](const auto *Alias) {
    // Skip inter-procedural aliases
    const auto *AliasFun = getFunction(Alias);
    if (AliasFun && Fun != AliasFun) {
      return;
    }

    if (V == Alias) {
      WithAlias(Alias);
      return;
    }

    if (llvm::isa<llvm::ConstantExpr, llvm::ConstantData>(Alias)) {
      // Assume: Compile-time constants are not generated as data-flow facts!
      return;
    }

    const auto *AliasBase = Alias->stripPointerCastsAndAliases();

    if (mustNoalias(Base, AliasBase)) {
      return;
    }

    WithAlias(Alias);
  });
}

void FilteredLLVMAliasIterator::forallAliasesOf(
    const llvm::Value *V, const llvm::Function *Fun,
    llvm::function_ref<void(const llvm::Value *)> WithAlias) {
  forallAliasesOfImpl(Underlying, V, nullptr, Fun, WithAlias);
}

void FilteredLLVMAliasIterator::forallAliasesOf(
    const llvm::Value *V, const llvm::Instruction *At,
    llvm::function_ref<void(const llvm::Value *)> WithAlias) {
  forallAliasesOfImpl(Underlying, V, At, At ? At->getFunction() : nullptr,
                      WithAlias);
}
