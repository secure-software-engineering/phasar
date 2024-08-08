#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/Utils/Logger.h"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"

using namespace psr;

static std::vector<const llvm::Function *> getVirtualFunctions(
    const llvm::StringMap<const llvm::GlobalVariable *> &ClearNameTVMap,
    const llvm::StructType &Type) {
  auto ClearName = LLVMTypeHierarchy::removeStructOrClassPrefix(Type.getName());

  auto It = ClearNameTVMap.find(ClearName);

  if (It != ClearNameTVMap.end()) {
    if (const auto *TI = llvm::dyn_cast<llvm::GlobalVariable>(It->second)) {
      if (!TI->hasInitializer()) {
        PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMTypeHierarchy",
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
    if (LLVMTypeHierarchy::isVTable(Glob.getName())) {
      auto Demang = llvm::demangle(Glob.getName().str());
      auto ClearName = LLVMTypeHierarchy::removeVTablePrefix(Demang);
      ClearNameTVMap.try_emplace(ClearName, &Glob);
    }
  }

  for (const auto *Ty : StructTypes) {
    TypeVFTMap.try_emplace(Ty, getVirtualFunctions(ClearNameTVMap, *Ty));
  }
}

LLVMVFTableProvider::LLVMVFTableProvider(const LLVMProjectIRDB &IRDB)
    : LLVMVFTableProvider(*IRDB.getModule()) {}

bool LLVMVFTableProvider::hasVFTable(const llvm::StructType *Type) const {
  return TypeVFTMap.count(Type);
}

const LLVMVFTable *
LLVMVFTableProvider::getVFTableOrNull(const llvm::StructType *Type) const {
  auto It = TypeVFTMap.find(Type);
  return It != TypeVFTMap.end() ? &It->second : nullptr;
}
