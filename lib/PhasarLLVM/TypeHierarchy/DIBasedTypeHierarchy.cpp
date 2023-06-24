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
#include "phasar/TypeHierarchy/VFTable.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

namespace psr {

DIBasedTypeHierarchy::DIBasedTypeHierarchy(const LLVMProjectIRDB &IRDB) {
  llvm::DebugInfoFinder Finder;

  const auto *const Module = IRDB.getModule();
  Finder.processModule(*Module);

  // find and save all base types first, so they are present in TypeToVertex
  size_t Counter = 0;
  for (const llvm::DIType *DIType : Finder.types()) {
    if (const auto *CompositeType =
            llvm::dyn_cast<llvm::DICompositeType>(DIType)) {
      TypeToVertex.try_emplace(CompositeType, Counter++);
      VertexTypes.push_back(CompositeType);
      continue;
    }
  }

  // find and save all derived types
  Counter = 0;
  for (const llvm::DIType *DIType : Finder.types()) {
    if (const auto *DerivedType = llvm::dyn_cast<llvm::DIDerivedType>(DIType)) {
      if (DerivedType->getTag() == llvm::dwarf::DW_TAG_inheritance) {
        // TypeToVertex.try_emplace(DerivedType, Counter++);

        llvm::SmallVector<size_t> BaseType = {
            Counter++, TypeToVertex[DerivedType->getBaseType()]};
        // VertexTypes.push_back(DerivedType);
        DerivedTypesOf.push_back(BaseType);
        continue;
      }
    }
  }

  // Initialize the transitive closure matrix with all as false
  std::vector<bool> InitVector(VertexTypes.size(), false);

  for (size_t I = 0; I < VertexTypes.size(); I++) {
    TransitiveClosure.push_back(InitVector);
  }

  // set edges
  for (const auto &Vertex : DerivedTypesOf) {
    TransitiveClosure[Vertex[0]][Vertex[1]] = true;
  }

  // Add transitive edges
  bool Change = true;
  size_t TCSize = TransitiveClosure.size();
  size_t RowIndex = 0;
  size_t PreviousRow = 0;
  // (max): I know the code below is very ugly, but I wanted to avoid recursion
  while (Change) {
    Change = false;
    for (size_t CurrentRow = 0; CurrentRow < TCSize; CurrentRow++) {
      for (size_t CompareRow = 0; CompareRow < TCSize; CompareRow++) {
        if (CurrentRow == CompareRow) {
          continue;
        }
        bool IsBaseType = false;
        for (size_t Column = 0; Column < TCSize; Column++) {
          if (TransitiveClosure[CurrentRow][Column]) {
            IsBaseType = true;
            break;
          }
        }

        if (!IsBaseType) {
          continue;
        }

        for (size_t Column = 0; Column < TCSize; Column++) {
          if (TransitiveClosure[CompareRow][Column] &&
              !TransitiveClosure[CurrentRow][Column] && CurrentRow != Column) {
            TransitiveClosure[CurrentRow][Column] = true;
            Change = true;
          }
        }
      }
    }
  }
}

[[nodiscard]] bool DIBasedTypeHierarchy::isSubType(ClassType Type,
                                                   ClassType SubType) {
  // find index of super type
  unsigned long IndexOfType = TypeToVertex.find(Type)->getSecond();
  unsigned long IndexOfSubType = TypeToVertex.find(SubType)->getSecond();

  // if the super type or the sub type haven't been found, return false
  if (IndexOfType >= TypeToVertex.size() ||
      IndexOfSubType >= TypeToVertex.size()) {
    return false;
  }

  return TransitiveClosure[IndexOfType][IndexOfSubType];
}

[[nodiscard]] auto DIBasedTypeHierarchy::getSubTypes(ClassType Type)
    -> std::set<ClassType> {
  // find index of super type
  unsigned long IndexOfType = TypeToVertex.find(Type)->getSecond();

  // if the super type hasn't been found, return an empty set
  if (IndexOfType >= TypeToVertex.size()) {
    return {};
  }

  // return all sub types
  std::set<ClassType> SubTypes = {};

  size_t Index = 0;
  for (const auto &Current : TransitiveClosure[IndexOfType]) {
    if (Current) {
      SubTypes.insert(VertexTypes[Index]);
    }

    Index++;
  }

  return SubTypes;
}

[[nodiscard]] bool DIBasedTypeHierarchy::isSuperType(ClassType Type,
                                                     ClassType SuperType) {
  return isSubType(SuperType, Type); // NOLINT
}

[[nodiscard]] auto DIBasedTypeHierarchy::getSuperTypes(ClassType Type)
    -> std::set<ClassType> {
  // TODO: implement (low priority)
  llvm::report_fatal_error("Not implemented");
}

[[nodiscard]] bool DIBasedTypeHierarchy::hasVFTable(ClassType Type) const {
  return Type->isVirtual();
}

[[nodiscard]] auto DIBasedTypeHierarchy::getVFTable(ClassType Type) const
    -> const VFTable<f_t> * {
  /// TODO: implement
  // Use the VTables deque here; either you have that pre-computed, or you
  // create it on demand

  // Problem: getting VFTables from Metadata nodes seems to be not possible?
  // Therefore, creating VTables seems not possible aswell
  // I will search for a solution though

  // schauen, ob Name mit z.B. _ZTV7Derived anfängt
  // printAsDot() schreiben, Ergebnis soll Format haben wie
  // boost::write_graphviz()
  // minimale version
  // jede node sollte namen haben

  // return VTables.at(Type->getMetadataID());

  llvm::report_fatal_error("Not implemented");
}

void DIBasedTypeHierarchy::print(llvm::raw_ostream &OS) const {
  /// TODO: implement
  OS << "Size of Transitive Closure: " << TransitiveClosure.size() << "\n";
  OS << "Number of Vertices " << VertexTypes.size() << "\n";
  OS << "Type Hierarchy:\n";

  size_t VertexIndex = 0;
  size_t EdgeIndex = 0;

  OS << "Transitive Closure Matrix\n";

  for (const auto &BaseType : TransitiveClosure) {
    for (const auto &Edge : BaseType) {
      OS << Edge << " ";
    }
    OS << "\n";
  }

  OS << "Names of vertex types\n";

  int Count = 0;
  for (const auto &Vertex : VertexTypes) {
    OS << Count << ": " << Vertex->getName() << "\n";
    Count++;
  }

  OS << "Derived Types size: " << DerivedTypesOf.size() << "\n";
  OS << "Derived Types:"
     << "\n";
  Count = 0;
  for (const auto &Derived : DerivedTypesOf) {
    for (const auto Curr : Derived) {
      OS << Curr << ", ";
    }
    OS << "\n";
  }

  OS << "Type Hierarchy\n";
  for (const auto &CurrentVertex : TransitiveClosure) {
    OS << VertexTypes[VertexIndex]->getName() << "\n";
    for (const bool CurrentEdge : CurrentVertex) {
      if (EdgeIndex != VertexIndex && CurrentEdge) {
        OS << "--> " << VertexTypes[EdgeIndex]->getName() << "\n";
      }
      EdgeIndex++;
    }
    VertexIndex++;
    EdgeIndex = 0;
  }

  OS << "VFTables:\n";
  // TODO: implement

  llvm::report_fatal_error("Not implemented");
}

[[nodiscard]] nlohmann::json DIBasedTypeHierarchy::getAsJson() const {
  /// TODO: implement
  llvm::report_fatal_error("Not implemented");
}
} // namespace psr
