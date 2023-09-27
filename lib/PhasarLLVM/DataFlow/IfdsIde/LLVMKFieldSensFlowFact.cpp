#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMKFieldSensFlowFact.h"

#include "llvm/IR/Instructions.h"

#include <optional>
#include <utility>

using namespace psr;

namespace psr {

std::optional<int64_t> getConstantOffset(const llvm::GetElementPtrInst *Gep) {
  if (!Gep->hasAllConstantIndices()) {
    return std::nullopt;
  }
  const auto &DL = Gep->getModule()->getDataLayout();
  llvm::APInt AccumulatedOffset(DL.getPointerSize() * 8, 0, true);
  Gep->accumulateConstantOffset(DL, AccumulatedOffset);
  return AccumulatedOffset.getSExtValue();
}

std::pair<const llvm::Value *, std::optional<int64_t>>
getAllocaInstAndConstantOffset(const llvm::GetElementPtrInst *Gep) {
  if (!Gep) {
    return {nullptr, std::nullopt};
  }
  auto CumulatedOffset = getConstantOffset(Gep);
  const auto *Alloca = Gep->getPointerOperand();
  bool Changed = false;
  do {
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
