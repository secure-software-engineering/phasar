#include "phasar/PhasarLLVM/Utils/AllocatedTypes.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

static const llvm::DICompositeType *
isCompositeStructType(const llvm::DIType *Ty) {
  if (const auto *CompTy = llvm::dyn_cast_if_present<llvm::DICompositeType>(Ty);
      CompTy && (CompTy->getTag() == llvm::dwarf::DW_TAG_structure_type ||
                 CompTy->getTag() == llvm::dwarf::DW_TAG_class_type)) {

    return CompTy;
  }

  return nullptr;
}

std::vector<const llvm::DICompositeType *>
psr::collectAllocatedTypes(const llvm::Module &Mod) {
  llvm::DenseSet<const llvm::DICompositeType *> AllocatedTypes;

  for (const auto &Fun : Mod) {
    for (const auto &Inst : llvm::instructions(Fun)) {
      if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&Inst)) {
        if (const auto *Ty = isCompositeStructType(getVarTypeFromIR(Alloca))) {
          AllocatedTypes.insert(Ty);
        }
      } else if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst)) {
        if (const auto *Callee = llvm::dyn_cast<llvm::Function>(
                Call->getCalledOperand()->stripPointerCastsAndAliases())) {
          if (psr::isHeapAllocatingFunction(Callee)) {
            const auto *MDNode = Call->getMetadata("heapallocsite");
            if (const auto *CompTy = llvm::
#if LLVM_VERSION_MAJOR >= 15
                    dyn_cast_if_present
#else
                    dyn_cast_or_null
#endif
                <llvm::DICompositeType>(MDNode);
                isCompositeStructType(CompTy)) {

              AllocatedTypes.insert(CompTy);
            }
          }
        }
      }
    }
  }

  std::vector<const llvm::DICompositeType *> AllocatedCompositeTypes;
  AllocatedCompositeTypes.reserve(AllocatedTypes.size());
  AllocatedCompositeTypes.insert(AllocatedCompositeTypes.end(),
                                 AllocatedTypes.begin(), AllocatedTypes.end());
  return AllocatedCompositeTypes;
}
