#ifndef PHASAR_PHASARLLVM_POINTER_SVFUTILS_H
#define PHASAR_PHASARLLVM_POINTER_SVFUTILS_H

#include "SVF-LLVM/LLVMModule.h"
#include "SVFIR/SVFIR.h"
#include "Util/GeneralType.h"

namespace psr {

[[nodiscard]] inline const llvm::Value *
svfVarToLLVMOrNull(const SVF::SVFVar *Var, SVF::LLVMModuleSet &ModSet) {
  if (ModSet.hasLLVMValue(Var)) {
    return ModSet.getLLVMValue(Var);
  }

  return nullptr;
}

[[nodiscard]] inline const llvm::Value *
pointerNodeToLLVMOrNull(SVF::NodeID Nod, SVF::LLVMModuleSet &ModSet,
                        SVF::SVFIR &PAG) {

  if (const SVF::SVFVar *Var = PAG.getGNode(Nod)) {
    return svfVarToLLVMOrNull(Var, ModSet);
  }
  return nullptr;
}

[[nodiscard]] inline const llvm::Value *
objectNodeToLLVMOrNull(SVF::NodeID Nod, SVF::LLVMModuleSet &ModSet,
                       SVF::SVFIR &PAG) {
  return pointerNodeToLLVMOrNull(Nod, ModSet, PAG);
}

[[nodiscard]] inline SVF::NodeID getNodeId(const llvm::Value *Pointer,
                                           SVF::LLVMModuleSet &ModSet) {
  return ModSet.getValueNode(Pointer);
}
[[nodiscard]] inline SVF::NodeID getObjNodeId(const llvm::Value *Obj,
                                              SVF::LLVMModuleSet &ModSet) {
  return ModSet.getObjectNode(Obj);
}

} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_SVFUTILS_H
