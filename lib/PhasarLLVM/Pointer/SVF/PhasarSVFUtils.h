#ifndef PHASAR_PHASARLLVM_POINTER_SVFUTILS_H
#define PHASAR_PHASARLLVM_POINTER_SVFUTILS_H

#include "SVF-LLVM/LLVMModule.h"
#include "SVFIR/SVFIR.h"
#include "Util/GeneralType.h"

namespace psr {
[[nodiscard]] inline const llvm::Value *
pointerNodeToLLVMOrNull(SVF::NodeID Nod, SVF::LLVMModuleSet &ModSet,
                        SVF::SVFIR &PAG) {

  if (const SVF::SVFVar *Var = PAG.getGNode(Nod)) {
    if (const auto *Val = Var->getValue()) {
      if (const auto *LLVMVal = ModSet.getLLVMValue(Val)) {
        return LLVMVal;
      }
    }
  }
  return nullptr;
}

[[nodiscard]] inline const llvm::Value *
objectNodeToLLVMOrNull(SVF::NodeID Nod, SVF::LLVMModuleSet &ModSet,
                       SVF::SVFIR &PAG) {
  if (const SVF::MemObj *Mem = PAG.getObject(Nod)) {
    if (const auto *Val = Mem->getValue()) {
      if (const auto *LLVMVal = ModSet.getLLVMValue(Val)) {
        return LLVMVal;
      }
    }
  }
  return nullptr;
}

[[nodiscard]] inline SVF::NodeID getNodeId(const llvm::Value *Pointer,
                                           SVF::LLVMModuleSet &ModSet,
                                           SVF::SVFIR &PAG) {
  auto *Nod = ModSet.getSVFValue(Pointer);
  return PAG.getValueNode(Nod);
}
[[nodiscard]] inline SVF::NodeID getObjNodeId(const llvm::Value *Obj,
                                              SVF::LLVMModuleSet &ModSet,
                                              SVF::SVFIR &PAG) {
  auto *Nod = ModSet.getSVFValue(Obj);
  return PAG.getObjectNode(Nod);
}

} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_SVFUTILS_H
