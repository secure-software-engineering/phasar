#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMKFieldSensFlowFact.h"

#include "llvm/IR/Instructions.h"

#include <optional>
#include <utility>

using namespace psr;

namespace psr {

std::optional<int64_t> getConstantOffset(const llvm::Value *GepOrBC) {
  if (llvm::isa<llvm::BitCastInst>(GepOrBC)) {
    return 0;
  }
  if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(GepOrBC)) {
    if (!Gep->hasAllConstantIndices()) {
      return std::nullopt;
    }
    const auto &DL = Gep->getModule()->getDataLayout();
    llvm::APInt AccumulatedOffset(DL.getPointerSize() * 8, 0, true);
    Gep->accumulateConstantOffset(DL, AccumulatedOffset);
    return AccumulatedOffset.getSExtValue();
  }
  return std::nullopt;
}

std::pair<const llvm::Value *, std::optional<int64_t>>
getAllocaInstAndConstantOffset(const llvm::Value *GepOrBC) {
  if (!GepOrBC) {
    return {nullptr, std::nullopt};
  }
  std::optional<int64_t> CumulatedOffset = 0;
  const auto *Alloca = GepOrBC;
  bool Changed = false;
  do {
    Changed = false;
    while (const auto *ChainedGEP =
               llvm::dyn_cast<llvm::GetElementPtrInst>(Alloca)) {
      if (CumulatedOffset.has_value()) {
        const auto ChainedGepOffset = getConstantOffset(ChainedGEP);
        if (ChainedGepOffset.has_value()) {
          CumulatedOffset = CumulatedOffset.value() + ChainedGepOffset.value();
        } else {
          CumulatedOffset = std::nullopt;
        }
      }
      Alloca = ChainedGEP->getPointerOperand();
      Changed = true;
    }
    if (const auto *ChainedBC = llvm::dyn_cast<llvm::BitCastInst>(Alloca)) {
      Alloca = ChainedBC->getOperand(0);
      Changed = true;
    }
  } while (Changed);
  return {Alloca, CumulatedOffset};
}

} // namespace psr
