#include "phasar/Utils/OpaquePtrTypeMap.h"

#include "llvm/IR/Instructions.h"

namespace psr {

OpaquePtrTypeInfoMap::OpaquePtrTypeInfoMap(const psr::LLVMProjectIRDB *Code) {
  for (const auto &Instr : Code->getAllInstructions()) {
    if (Instr->getType()->isOpaquePointerTy()) {
      TypeInfo.try_emplace(Instr->getOpcodeName());
    }
  }

  for (const auto &Instr : Code->getAllInstructions()) {
    if (const auto &Store = llvm::dyn_cast<llvm::StoreInst>(Instr)) {
      const auto &Operand = Store->getPointerOperand();

      if (Operand->getType()->isPointerTy()) {
        TypeInfo[Store->getOpcodeName()] = Store->getValueOperand()->getName();
      }
    }
  }
};

} // namespace psr