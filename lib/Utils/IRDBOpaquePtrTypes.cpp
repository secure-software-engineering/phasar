#include "phasar/Utils/IRDBOpaquePtrTypes.h"

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

namespace psr {

IRDBOpaquePtrTypes::IRDBOpaquePtrTypes(const LLVMProjectIRDB *IRDB) {
  llvm::DebugInfoFinder DIF;
  const auto *M = IRDB->getModule();
  DIF.processModule(*M);

  for (const auto *Instr : IRDB->getAllInstructions()) {
    if (Instr->getType()->isOpaquePointerTy()) {
      llvm::outs() << "is opaque pointer\n";
    }
  }

  // Map values to DITypes
  for (const auto *Instr : IRDB->getAllInstructions()) {
    // Check if the current instruction has debug information
    if (const auto *DbgDeclare = llvm::dyn_cast<llvm::DbgDeclareInst>(Instr)) {
      if (auto *Var = llvm::dyn_cast<llvm::DILocalVariable>(
              DbgDeclare->getVariable())) {
        for (const auto *DIFType : DIF.types()) {
          if (Var->getType() == DIFType) {
            ValueToDIType.try_emplace(DbgDeclare->getAddress(), DIFType);
          }
        }
      }
    }
  }

  // Map local variables to their according subprograms
  for (const auto *DITy : DIF.types()) {
    if (const auto *Var = llvm::dyn_cast<llvm::DILocalVariable>(DITy)) {
      llvm::outs() << "cast to localVar successful\n";
      if (SubprogamVars.find(Var->getScope()->getSubprogram()) ==
          SubprogamVars.end()) {
        std::vector<const llvm::DILocalVariable *> InitVector;
        SubprogamVars.try_emplace(Var->getScope()->getSubprogram(),
                                  std::move(InitVector));
      }
      SubprogamVars[Var->getScope()->getSubprogram()].push_back(Var);
    }
  }

  // Map pointers to the type they are pointing to
  for (const auto *Instr : IRDB->getAllInstructions()) {
    if (const auto *Value = llvm::dyn_cast<llvm::Value>(Instr)) {
      if (Value->getType()->isOpaquePointerTy()) {
        ValueToType.try_emplace(Value, getPtrType(Value));
      }
    }
  }

  llvm::outs() << "ValueToDIType map:\n";
  for (const auto &[Key, Val] : ValueToDIType) {
    llvm::outs() << "Key: " << *Key << " Value: " << *Val << "\n";
  }

  llvm::outs() << "DITypeToValue map:\n";
  for (const auto &[Key, Val] : DITypeToValue) {
    llvm::outs() << "Key: " << *Key << " Value: " << *Val << "\n";
  }

  llvm::outs() << "\nSubprogamVars map:\n";
  for (const auto &[Key, Val] : SubprogamVars) {
    llvm::outs() << "Key: " << Key << " Values\n{";
    for (const auto &Elem : Val) {
      llvm::outs() << Elem << ", ";
    }
    llvm::outs() << "}\n";
  }
  llvm::outs() << "\nValueToType map:\n";
  for (const auto &[Key, Val] : ValueToType) {
    llvm::outs() << "Key: " << Key << " Value: " << Val << "\n";
  }
}

const llvm::Type *IRDBOpaquePtrTypes::getTypeOfPtr(const llvm::Value *Value) {
  return ValueToType[Value];
}

const llvm::DIType *
IRDBOpaquePtrTypes::getBaseType(const llvm::DIDerivedType *DerivedTy) {
  if (const auto *BaseTy =
          llvm::dyn_cast<llvm::DIBasicType>(DerivedTy->getBaseType())) {
    return BaseTy;
  };

  if (const auto *CompTy =
          llvm::dyn_cast<llvm::DICompositeType>(DerivedTy->getBaseType())) {
    return CompTy;
  }

  // case: getBaseType() is a derived type itself
  if (const auto *BaseIsDerivedTy =
          llvm::dyn_cast<llvm::DIDerivedType>(DerivedTy->getBaseType())) {
    return getBaseType(BaseIsDerivedTy);
  }

  // case: getBaseType() returns a DISubroutineType, in which we need to
  // find the type inside the corresponding subprogram
  /*
  if (const auto *BaseIsInSubprogram =
          llvm::dyn_cast<llvm::DISubroutineType>(DerivedTy)) {
    return;
  }*/
  llvm::report_fatal_error("No base type found");
}

const llvm::Type *IRDBOpaquePtrTypes::getPtrType(const llvm::Value *Value) {
  const auto *CorrespondingDIType = ValueToDIType[Value];
  if (const auto *DerivedTyTy =
          llvm::dyn_cast<llvm::DIDerivedType>(CorrespondingDIType)) {
    return DITypeToValue[getBaseType(DerivedTyTy)]->getType();
  }
  llvm::report_fatal_error("Value is not a derived type");
}

} // namespace psr
