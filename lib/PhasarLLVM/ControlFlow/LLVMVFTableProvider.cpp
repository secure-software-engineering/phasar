#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/Logger.h"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"

using namespace psr;

static std::vector<const llvm::Function *> getVirtualFunctions(
    const llvm::StringMap<const llvm::GlobalVariable *> &ClearNameTVMap,
    const llvm::StructType &Type) {
  auto ClearName =
      DIBasedTypeHierarchy::removeStructOrClassPrefix(Type.getName());

  auto It = ClearNameTVMap.find(ClearName);

  if (It != ClearNameTVMap.end()) {
    if (const auto *TI = llvm::dyn_cast<llvm::GlobalVariable>(It->second)) {
      if (!TI->hasInitializer()) {
        PHASAR_LOG_LEVEL_CAT(DEBUG, "DIBasedTypeHierarchy",
                             ClearName << " does not have initializer");
        return {};
      }
      if (const auto *I =
              llvm::dyn_cast<llvm::ConstantStruct>(TI->getInitializer())) {
        return LLVMVFTable::getVFVectorFromIRVTable(*I);
      }
    }
  }
  return {};
}

static std::vector<const llvm::Function *> getVirtualFunctionsDIBased(
    const llvm::StringMap<const llvm::GlobalVariable *> &ClearNameTVMap,
    const llvm::DIType &Type) {
  auto ClearName =
      DIBasedTypeHierarchy::removeStructOrClassPrefix(Type.getName());

  auto It = ClearNameTVMap.find(ClearName);

  if (It != ClearNameTVMap.end()) {
    if (const auto *TI = llvm::dyn_cast<llvm::GlobalVariable>(It->second)) {
      if (!TI->hasInitializer()) {
        PHASAR_LOG_LEVEL_CAT(DEBUG, "DIBasedTypeHierarchy",
                             ClearName << " does not have initializer");
        return {};
      }
      if (const auto *I =
              llvm::dyn_cast<llvm::ConstantStruct>(TI->getInitializer())) {
        return LLVMVFTable::getVFVectorFromIRVTable(*I);
      }
    }
  }
  return {};
}

LLVMVFTableProvider::LLVMVFTableProvider(const llvm::Module &Mod) {
  auto StructTypes = Mod.getIdentifiedStructTypes();

  llvm::StringMap<const llvm::GlobalVariable *> ClearNameTVMap;

  for (const auto &Glob : Mod.globals()) {
    if (DIBasedTypeHierarchy::isVTable(Glob.getName())) {
      auto Demang = llvm::demangle(Glob.getName().str());
      auto ClearName = DIBasedTypeHierarchy::removeVTablePrefix(Demang);
      ClearNameTVMap.try_emplace(ClearName, &Glob);
    }
  }

  for (const auto *Ty : StructTypes) {
    TypeVFTMap.try_emplace(Ty, getVirtualFunctions(ClearNameTVMap, *Ty));
  }
}

LLVMVFTableProvider::LLVMVFTableProvider(const LLVMProjectIRDB &IRDB)
    : LLVMVFTableProvider(*IRDB.getModule()) {
  for (const auto *Instr : IRDB.getAllInstructions()) {
    if (const auto *Val = llvm::dyn_cast<llvm::Value>(Instr)) {
      if (const auto *DILocalVar = getDILocalVariable(Val)) {
        if (const auto *DerivedTy =
                llvm::dyn_cast<llvm::DIDerivedType>(DILocalVar->getType())) {
          DITypeToType[DerivedTy->getBaseType()] = Val->getType();
          continue;
        }
        DITypeToType[DILocalVar->getType()] = Val->getType();
      }
    }
  }
  llvm::StringMap<const llvm::GlobalVariable *> ClearNameTVMap;

  for (const auto &Glob : IRDB.getModule()->globals()) {
    if (DIBasedTypeHierarchy::isVTable(Glob.getName())) {
      auto Demang = llvm::demangle(Glob.getName().str());
      auto ClearName = DIBasedTypeHierarchy::removeVTablePrefix(Demang);
      ClearNameTVMap.try_emplace(ClearName, &Glob);
    }
  }

  for (const auto &Elem : DITypeToType) {
    DITypeVFTMap.try_emplace(
        Elem.first, getVirtualFunctionsDIBased(ClearNameTVMap, *Elem.first));
  }
}

bool LLVMVFTableProvider::hasVFTable(const llvm::StructType *Type) const {
  return TypeVFTMap.count(Type);
}

bool LLVMVFTableProvider::hasVFTable(const llvm::DIType *Type) const {
  return DITypeToType.count(Type);
}

const LLVMVFTable *
LLVMVFTableProvider::getVFTableOrNull(const llvm::StructType *Type) const {
  auto It = TypeVFTMap.find(Type);
  return It != TypeVFTMap.end() ? &It->second : nullptr;
}
