/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchyData.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace psr {
using ClassType = DIBasedTypeHierarchy::ClassType;

static std::vector<std::vector<const llvm::Function *>>
buildVTables(const llvm::DebugInfoFinder &DIF,
             llvm::ArrayRef<const llvm::DICompositeType *> VertexTypes,
             const llvm::DenseMap<ClassType, size_t> &TypeToVertex,
             const LLVMProjectIRDB &IRDB) {
  std::vector<std::vector<const llvm::Function *>> VT(VertexTypes.size());

  for (const auto *DIFun : DIF.subprograms()) {
    auto Virt = DIFun->getVirtuality();
    if (!Virt) {
      continue;
    }
    auto VIdx = DIFun->getVirtualIndex();
    auto *Parent = llvm::dyn_cast<llvm::DICompositeType>(DIFun->getScope());
    if (!Parent) {
      continue;
    }
    auto IdxIt = TypeToVertex.find(Parent);
    if (IdxIt == TypeToVertex.end()) [[unlikely]] {
      PHASAR_LOG_LEVEL(WARNING,
                       "Enclosing type '"
                           << Parent->getName() << "' of virtual function '"
                           << llvm::demangle(DIFun->getLinkageName().str())
                           << "'  not found in the current module");

      continue;
    }

    const auto *Fun = IRDB.getFunction(DIFun->getLinkageName());
    if (!Fun) {
      PHASAR_LOG_LEVEL(WARNING,
                       "Referenced virtual function '"
                           << llvm::demangle(DIFun->getLinkageName().str())
                           << "' (aka. " << DIFun->getLinkageName()
                           << ") not declared in the current module");
      continue;
    }

    auto &VTable = VT[IdxIt->second];
    if (VTable.size() == VIdx) {
      VTable.push_back(Fun);
    } else {
      if (VTable.size() < VIdx) {
        VTable.resize(VIdx + 1);
      }
      VTable[VIdx] = Fun;
    }
  }
  return VT;
}

struct TypeGraph {
  llvm::SmallBitVector Roots;
  llvm::SmallVector<llvm::SmallVector<uint32_t>> DerivedTypesOf;
};

static TypeGraph
buildTypeGraph(llvm::ArrayRef<const llvm::DICompositeType *> VertexTypes,
               const llvm::DenseMap<ClassType, size_t> &TypeToVertex) {
  TypeGraph TG{llvm::SmallBitVector(VertexTypes.size(), true), {}};
  TG.DerivedTypesOf.resize(VertexTypes.size());

  for (const auto *Composite : VertexTypes) {
    auto DerivedIdx = TypeToVertex.lookup(Composite);
    assert(DerivedIdx != 0 || VertexTypes[0] == Composite);

    for (const auto *Fld : Composite->getElements()) {
      if (const auto *Inheritenace = llvm::dyn_cast<llvm::DIDerivedType>(Fld);
          Inheritenace &&
          Inheritenace->getTag() == llvm::dwarf::DW_TAG_inheritance) {
        auto BaseIdx = TypeToVertex.lookup(Inheritenace->getBaseType());
        assert(BaseIdx != 0 || VertexTypes[0] == Inheritenace->getBaseType());

        TG.DerivedTypesOf[BaseIdx].push_back(DerivedIdx);
        TG.Roots.reset(DerivedIdx);
      }
    }
  }

  return TG;
}

static void buildTypeHierarchy(
    const TypeGraph &TG,
    llvm::ArrayRef<const llvm::DICompositeType *> VertexTypes,
    std::vector<std::pair<uint32_t, uint32_t>> &TransitiveDerivedIndex,
    std::vector<ClassType> &Hierarchy) {
  TransitiveDerivedIndex.resize(VertexTypes.size());

  llvm::SmallBitVector Seen(VertexTypes.size());

  llvm::SmallVector<int32_t> WorkList;

  for (uint32_t Rt : TG.Roots.set_bits()) {
    WorkList.emplace_back(Rt);

    while (!WorkList.empty()) {
      auto Curr = WorkList.pop_back_val();

      if (Curr < 0) {
        // Pop N elements
        auto TypeIdx = ~Curr;
        TransitiveDerivedIndex[TypeIdx].second = Hierarchy.size();
        continue;
      }

      if (!Seen.test(Curr)) {
        TransitiveDerivedIndex[Curr].first = Hierarchy.size();
      } else {
        Seen.set(Curr);
      }
      Hierarchy.push_back(VertexTypes[Curr]);
      // llvm::errs() << " -- push " << VertexTypes[Curr]->getName() << '\n';
      WorkList.push_back(~Curr);
      WorkList.append(TG.DerivedTypesOf[Curr].begin(),
                      TG.DerivedTypesOf[Curr].end());
    }
  }
}

DIBasedTypeHierarchy::DIBasedTypeHierarchy(const LLVMProjectIRDB &IRDB) {
  // -- Find all types
  {
    llvm::DebugInfoFinder DIF;
    DIF.processModule(*IRDB.getModule());
    {
      size_t NumTypes = DIF.type_count(); // upper bound

      TypeToVertex.reserve(NumTypes);
      VertexTypes.reserve(NumTypes);
    }

    // -- Filter all struct- or class types

    for (const auto *Ty : DIF.types()) {
      if (const auto *Composite = llvm::dyn_cast<llvm::DICompositeType>(Ty)) {
        TypeToVertex.try_emplace(Composite, VertexTypes.size());
        VertexTypes.push_back(Composite);
        NameToType.try_emplace(Composite->getName(), Composite);
      }
    }

    // -- Construct VTables

    auto VT = buildVTables(DIF, VertexTypes, TypeToVertex, IRDB);
    VTables.assign(std::make_move_iterator(VT.begin()),
                   std::make_move_iterator(VT.end()));
  }

  // -- Build a type-graph
  auto TG = buildTypeGraph(VertexTypes, TypeToVertex);

  // -- Build the transitive closure
  buildTypeHierarchy(TG, VertexTypes, TransitiveDerivedIndex, Hierarchy);
}

static const llvm::DICompositeType *
stringToDICompositeType(const LLVMProjectIRDB *IRDB,
                        const llvm::StringRef DITypeName) {
  llvm::DebugInfoFinder DIF;
  const auto *Module = IRDB->getModule();
  DIF.processModule(*Module);

  for (const auto &Type : DIF.types()) {
    if (const auto *DICT = llvm::dyn_cast<llvm::DICompositeType>(Type)) {
      if (DICT->getName() == DITypeName) {
        return DICT;
      }

      if (DICT->getIdentifier() == DITypeName) {
        return DICT;
      }
    }
  }

  llvm::report_fatal_error("DIType doesn't exist");
}

static const llvm::DIType *stringToDIType(const LLVMProjectIRDB *IRDB,
                                          const llvm::StringRef DITypeName) {
  llvm::DebugInfoFinder DIF;
  const auto *Module = IRDB->getModule();
  DIF.processModule(*Module);

  for (const auto &Type : DIF.types()) {
    if (Type) {
      if (Type->getName() == DITypeName) {
        if (const auto *DIT = llvm::dyn_cast<llvm::DIType>(Type)) {
          return DIT;
        }
      }
    }
  }

  llvm::report_fatal_error("DIType doesn't exist");
}

DIBasedTypeHierarchy::DIBasedTypeHierarchy(
    const LLVMProjectIRDB *IRDB,
    const DIBasedTypeHierarchyData &SerializedData) {
  llvm::outs() << "Size: " << SerializedData.VertexTypes.size() << "\n";
  llvm::outs().flush();

  int Counter = 0;
  VertexTypes.reserve(SerializedData.VertexTypes.size());

  for (const auto &Curr : SerializedData.VertexTypes) {
    VertexTypes.push_back(stringToDICompositeType(IRDB, Curr));
    Counter++;
  }

  llvm::outs() << "Constr: " << Counter << "\n";
  llvm::outs() << "VertexTypes: " << VertexTypes.size() << "\n";
  llvm::outs().flush();

  for (const auto &Curr : SerializedData.NameToType) {
    NameToType.try_emplace(Curr.getKey(),
                           stringToDIType(IRDB, Curr.getValue()));
  }

  for (const auto &Curr : SerializedData.TypeToVertex) {
    TypeToVertex.try_emplace(stringToDIType(IRDB, Curr.getKey()),
                             Curr.getValue());
  }

  for (const auto &Curr : SerializedData.TransitiveDerivedIndex) {
    TransitiveDerivedIndex.emplace_back(
        std::pair<uint32_t, uint32_t>(Curr.first, Curr.second));
  }

  for (const auto &Curr : SerializedData.Hierarchy) {
    Hierarchy.push_back(stringToDIType(IRDB, Curr));
  }

  for (const auto &Curr : SerializedData.VTables) {
    LLVMVFTable CurrVTable = LLVMVFTable();

    for (const auto &FuncName : Curr) {
      CurrVTable.VFT.push_back(IRDB->getFunction(FuncName));
    }

    VTables.push_back(std::move(CurrVTable));
  }
}

auto DIBasedTypeHierarchy::subTypesOf(size_t TypeIdx) const noexcept
    -> llvm::iterator_range<const ClassType *> {
  const auto *Data = Hierarchy.data();
  auto [Start, End] = TransitiveDerivedIndex[TypeIdx];
  return {std::next(Data, Start), std::next(Data, End)};
}

auto DIBasedTypeHierarchy::subTypesOf(ClassType Ty) const noexcept
    -> llvm::iterator_range<const ClassType *> {
  auto It = TypeToVertex.find(Ty);
  if (It == TypeToVertex.end()) {
    const auto *Data = Hierarchy.data();
    return {Data, Data};
  }

  return subTypesOf(It->second);
}

[[nodiscard]] bool DIBasedTypeHierarchy::isSuperType(ClassType Type,
                                                     ClassType SuperType) {
  return isSubType(SuperType, Type); // NOLINT
}

[[nodiscard]] auto DIBasedTypeHierarchy::getSuperTypes(ClassType /*Type*/)
    -> std::set<ClassType> {
  // TODO: implement (low priority)
  llvm::report_fatal_error("Not implemented");
}

[[nodiscard]] bool DIBasedTypeHierarchy::hasVFTable(ClassType Type) const {
  const auto *StructTy = llvm::dyn_cast<llvm::DICompositeType>(Type);
  return StructTy && StructTy->getVTableHolder();
}

void DIBasedTypeHierarchy::print(llvm::raw_ostream &OS) const {
  {
    OS << "Type Hierarchy:\n";
    size_t TyIdx = 0;
    for (const auto *Ty : VertexTypes) {
      OS << Ty->getName() << " --> ";
      for (const auto *SubTy : llvm::drop_begin(subTypesOf(TyIdx))) {
        OS << SubTy->getName() << ' ';
      }
      ++TyIdx;
      OS << '\n';
    }
  }

  {
    size_t TyIdx = 0;
    OS << "VFTables:\n";

    for (const auto &VFT : VTables) {
      OS << "Virtual function table for: " << VertexTypes[TyIdx]->getName()
         << '\n';
      for (const auto *F : VFT) {
        OS << "\t-" << (F ? F->getName() : "<null>") << '\n';
      }
      ++TyIdx;
    }
  }
}

[[nodiscard]] [[deprecated("Please use printAsJson() instead")]] nlohmann::json
DIBasedTypeHierarchy::getAsJson() const {
  /// TODO: implement
  llvm::report_fatal_error("Not implemented");
}

DIBasedTypeHierarchyData DIBasedTypeHierarchy::getTypeHierarchyData() const {
  DIBasedTypeHierarchyData Data;
  for (const auto &Curr : NameToType) {
    Data.NameToType.try_emplace(Curr.getKey().str(),
                                Curr.getValue()->getName().str());
  }

  for (const auto &Curr : TypeToVertex) {
    Data.TypeToVertex.try_emplace(Curr.getFirst()->getName().str(),
                                  Curr.getSecond());
  }

  int Counter = 0;
  for (const auto &Curr : VertexTypes) {
    if (!Curr) {
      Data.VertexTypes.emplace_back("");
      llvm::errs() << "VertexType is null\n";
    }

    if (!Curr->getName().empty()) {
      Data.VertexTypes.push_back(Curr->getName().str());
      Counter++;
      llvm::outs() << "NAME: " << Curr->getName() << "\n";
      continue;
    }

    if (!Curr->getIdentifier().empty()) {
      Data.VertexTypes.push_back(Curr->getIdentifier().str());
      Counter++;
      llvm::outs() << "IDENTIFIER: " << Curr->getIdentifier() << "\n";
      continue;
    }

    Data.VertexTypes.emplace_back("");
    llvm::errs() << "VertexType has no valid name or identifier\n";
  }
  llvm::outs() << "getTypeHierarchyData: " << Counter << "\n";
  llvm::outs().flush();

  for (const auto &Curr : TransitiveDerivedIndex) {
    Data.TransitiveDerivedIndex.emplace_back(
        std::pair<uint32_t, uint32_t>(Curr.first, Curr.second));
  }

  for (const auto &Curr : Hierarchy) {
    Data.Hierarchy.push_back(Curr->getName().str());
  }

  for (const auto &Curr : VTables) {
    std::vector<std::string> CurrVTableAsString;
    CurrVTableAsString.reserve(Curr.getAllFunctions().size());

    for (const auto &Func : Curr.getAllFunctions()) {
      if (Func) {
        CurrVTableAsString.push_back(Func->getName().str());
        continue;
      }
      CurrVTableAsString.emplace_back("Null");
    }

    Data.VTables.push_back(std::move(CurrVTableAsString));
  }

  return Data;
}

void DIBasedTypeHierarchy::printAsJson(llvm::raw_ostream &OS) const {
  DIBasedTypeHierarchyData Data = getTypeHierarchyData();
  Data.printAsJson(OS);
}

void DIBasedTypeHierarchy::printAsDot(llvm::raw_ostream &OS) const {
  OS << "digraph TypeHierarchy{\n";
  scope_exit CloseBrace = [&OS] { OS << "}\n"; };

  // add nodes
  for (const auto &[Type, Vtx] : TypeToVertex) {
    OS << Vtx << "[label=\"";
    OS.write_escaped(Type->getName()) << "\"];\n";
  }

  // add all edges

  for (size_t I = 0, End = TypeToVertex.size(); I != End; ++I) {
    for (const auto &SubType : subTypesOf(I)) {
      OS << I << " -> " << TypeToVertex.lookup(SubType) << ";\n";
    }
  }
}

} // namespace psr
