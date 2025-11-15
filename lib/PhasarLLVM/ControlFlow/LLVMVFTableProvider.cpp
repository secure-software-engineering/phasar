#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/MapUtils.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"

using namespace psr;

static constexpr llvm::StringLiteral TSPrefixDemang = "typeinfo name for ";
static constexpr llvm::StringLiteral VTablePrefixDemang = "vtable for ";
static constexpr llvm::StringLiteral VTablePrefix = "_ZTV";

static std::string getTypeName(const llvm::DIType *DITy) {
  auto TypeName = [DITy] {
    if (const auto *CompTy = llvm::dyn_cast<llvm::DICompositeType>(DITy)) {
      if (auto Ident = CompTy->getIdentifier(); !Ident.empty()) {
        return Ident;
      }
    }
    return DITy->getName();
  }();

  // In LLVM 17 demangle() takes a StringRef
  auto Ret = llvm::demangle(TypeName.str());

  if (llvm::StringRef(Ret).startswith(TSPrefixDemang)) {
    Ret.erase(0, TSPrefixDemang.size());
  }

  return Ret;
}

static void insertVirtualFunctions(
    std::unordered_map<std::pair<const llvm::DIType *, uint32_t>, LLVMVFTable,
                       PairHash> &Into,
    const llvm::DIType *Type, const llvm::GlobalVariable *VTableGlobal) {
  if (!VTableGlobal) {
    return;
  }

  if (const auto *VT = llvm::dyn_cast<llvm::ConstantStruct>(
          VTableGlobal->getInitializer())) {
    auto NumElems = VT->getNumOperands();

    // llvm::errs() << "[insertVirtualFunctions]: VT: " << *VT << '\n';
    // llvm::errs() << "[insertVirtualFunctions]: >  NumElems: " << NumElems
    // << '\n';
    for (uint32_t I = 0; I != NumElems; ++I) {
      Into[{Type, I}] = LLVMVFTable::getVFVectorFromIRVTable(*VT, I);
    }
  }
}

static void getBasesOfVirt(
    llvm::SmallDenseMap<const llvm::DIType *, llvm::SmallDenseSet<uint32_t>>
        &Into,
    const llvm::DICompositeType *VirtTy, uint32_t CurrIdx = 0) {
  Into[VirtTy].insert(CurrIdx);
  for (const auto *Elem : VirtTy->getElements()) {
    const auto *Inher = llvm::dyn_cast<llvm::DIDerivedType>(Elem);
    if (!Inher) {
      // Inheritance is always at the front of the member-list
      break;
    }
    if (Inher->getTag() != llvm::dwarf::DW_TAG_inheritance) {
      continue;
    }

    const auto *BaseClass =
        llvm::dyn_cast<llvm::DICompositeType>(Inher->getBaseType());
    if (!BaseClass || !BaseClass->getVTableHolder()) {
      continue;
    }
    getBasesOfVirt(Into, BaseClass, CurrIdx);
    CurrIdx++;
  }
}

LLVMVFTableProvider::LLVMVFTableProvider(const llvm::Module &Mod) {
  for (const auto &Glob : Mod.globals()) {
    if (isVTable(Glob.getName())) {
      auto Demang = llvm::demangle(Glob.getName().str());
      auto ClearName = removeVTablePrefix(Demang);
      // llvm::errs() << "> ClearName: " << ClearName << '\n';
      ClearNameTVMap.try_emplace(ClearName, &Glob);
    }
  }

  llvm::DebugInfoFinder DIF;
  DIF.processModule(Mod);
  for (const auto *Ty : DIF.types()) {
    if (const auto *CompTy = llvm::dyn_cast<llvm::DICompositeType>(Ty)) {
      if (CompTy->getTag() == llvm::dwarf::DW_TAG_class_type ||
          CompTy->getTag() == llvm::dwarf::DW_TAG_structure_type) {
        insertVirtualFunctions(
            TypeVFTMap, CompTy,
            getOrDefault(ClearNameTVMap, getTypeName(CompTy)));

        if (CompTy->getVTableHolder()) {
          auto &BaseTys = BasesOfVirt[CompTy];
          getBasesOfVirt(BaseTys, CompTy);
        }
      }
    }
  }
}

LLVMVFTableProvider::LLVMVFTableProvider(const LLVMProjectIRDB &IRDB)
    : LLVMVFTableProvider(*IRDB.getModule()) {}

bool LLVMVFTableProvider::hasVFTable(const llvm::DIType *Type) const {
  return TypeVFTMap.count({Type, 0});
}

const LLVMVFTable *
LLVMVFTableProvider::getVFTableOrNull(const llvm::DIType *Type,
                                      uint32_t Index) const {
  auto It = TypeVFTMap.find({Type, Index});
  return It != TypeVFTMap.end() ? &It->second : nullptr;
}

const llvm::GlobalVariable *
LLVMVFTableProvider::getVFTableGlobal(const llvm::DIType *Type) const {
  auto Name = getTypeName(Type);
  return getVFTableGlobal(Name);
}

const llvm::GlobalVariable *
LLVMVFTableProvider::getVFTableGlobal(llvm::StringRef ClearTypeName) const {
  // llvm::errs() << "[getVFTableGlobal]: " << ClearTypeName << '\n';
  if (auto It = ClearNameTVMap.find(ClearTypeName);
      It != ClearNameTVMap.end()) {
    return It->second;
  }
  return nullptr;
}

static const auto &getDefaultIndices() {
  static const llvm::SmallDenseSet<uint32_t> DefaultIndices = {0};
  return DefaultIndices;
}

const llvm::SmallDenseSet<uint32_t> &
LLVMVFTableProvider::getVTableIndexInHierarchy(
    const llvm::DIType *DerivedType, const llvm::DIType *BaseType) const {
  auto OuterIt = BasesOfVirt.find(DerivedType);
  if (OuterIt == BasesOfVirt.end()) {
    return getDefaultIndices();
  }

  auto InnerIt = OuterIt->second.find(BaseType);
  if (InnerIt == OuterIt->second.end()) {
    return getDefaultIndices();
  }

  return InnerIt->second;
}

llvm::StringRef
LLVMVFTableProvider::removeVTablePrefix(llvm::StringRef GlobName) noexcept {
  if (GlobName.startswith(VTablePrefixDemang)) {
    return GlobName.drop_front(VTablePrefixDemang.size());
  }
  if (GlobName.startswith(VTablePrefix)) {
    return GlobName.drop_front(VTablePrefix.size());
  }
  return GlobName;
}

/// Supercedes DIBasedTypeHierarchy::isVTable() + removeVTablePrefix
bool LLVMVFTableProvider::isVTable(llvm::StringRef MangledVarName) {
  if (MangledVarName.startswith(VTablePrefix)) {
    return true;
  }
  // In LLVM 17 demangle() takes a StringRef
  auto Demang = llvm::demangle(MangledVarName.str());
  return llvm::StringRef(Demang).startswith(VTablePrefixDemang);
}
