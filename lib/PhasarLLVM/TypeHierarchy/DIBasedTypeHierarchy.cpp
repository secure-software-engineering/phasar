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
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>

#include <llvm-14/llvm/IR/DebugInfoMetadata.h>

namespace psr {

DIBasedTypeHierarchy::DIBasedTypeHierarchy(const LLVMProjectIRDB &IRDB) {
  const auto *const Module = IRDB.getModule();
  Finder.processModule(*Module);

  // find and save all base types first, so they are present in TypeToVertex
  size_t Counter = 0;
  for (const llvm::DIType *DIType : Finder.types()) {
    if (const auto *CompositeType =
            llvm::dyn_cast<llvm::DICompositeType>(DIType)) {
      NameToType.try_emplace(CompositeType->getName(), CompositeType);
      TypeToVertex.try_emplace(CompositeType, Counter++);
      VertexTypes.push_back(CompositeType);
      continue;
    }
  }

  std::vector<llvm::SmallVector<size_t>> DerivedTypesOf;
  // find and save all derived types
  for (const llvm::DIType *DIType : Finder.types()) {
    if (const auto *DerivedType = llvm::dyn_cast<llvm::DIDerivedType>(DIType)) {
      if (DerivedType->getTag() == llvm::dwarf::DW_TAG_inheritance) {
        const size_t ActualDerivedType =
            TypeToVertex[NameToType[DerivedType->getScope()->getName()]];

        // (max) assertions fail, but code works?!
        // assert(ActualDerivedType);

        size_t BaseTypeVertex = TypeToVertex[DerivedType->getBaseType()];
        // (max) assertions fail, but code works?!
        // assert(BaseTypeVertex);

        llvm::SmallVector<size_t> BaseType = {
            BaseTypeVertex, static_cast<size_t>(ActualDerivedType)};
        DerivedTypesOf.push_back(BaseType);
        continue;
      }
    }
  }

  // Initialize the transitive closure matrix with all as false
  llvm::BitVector InitVector(VertexTypes.size(), false);

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

        if (!TransitiveClosure[CurrentRow][CompareRow]) {
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

  // add edges onto vertices themselves
  for (size_t I = 0; I < TransitiveClosure.size(); I++) {
    TransitiveClosure[I][I] = true;
  }

  const auto Subprograms = Finder.subprograms();
  /*
  for (const auto *Current = Subprograms.begin(); Current != Subprograms.end();
       ++Current) {
    if (const auto *SubProgramType =
            llvm::dyn_cast<llvm::DISubprogram>(Current)) {
      if (SubProgramType->getVirtualIndex() == 0) {
        VTables.push_back(SubProgramType);
      }
    }
  }*/
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
  for (size_t I = 0; I < TransitiveClosure[IndexOfType].size(); I++) {
    if (TransitiveClosure[IndexOfType][I]) {
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

  // return VTables;

  llvm::report_fatal_error("Not implemented");
}

void DIBasedTypeHierarchy::print(llvm::raw_ostream &OS) const {

  // TODO (max): printAsDot() schreiben, Ergebnis soll Format haben wie
  // boost::write_graphviz()
  // minimale version
  // jede node sollte namen haben
  /*
  digraph G {
    start[label="base"];
    start -> a0;
    start -> b0;
    a1 -> b3;
    b2 -> a3;
    a3 -> a0;
    a3 -> end;
    b3 -> end;
  }
  */

  OS << "Size of Transitive Closure: " << TransitiveClosure.size() << "\n";
  OS << "Number of Vertices " << VertexTypes.size() << "\n";
  OS << "Type Hierarchy:\n";

  size_t VertexIndex = 0;
  size_t EdgeIndex = 0;

  OS << "Transitive Closure Matrix\n";

  OS << "Type Hierarchy\n";
  for (const auto &CurrentVertex : TransitiveClosure) {
    OS << VertexTypes[VertexIndex]->getName() << "\n";
    for (size_t I = 0; I < TransitiveClosure.size(); I++) {
      if (EdgeIndex != VertexIndex && CurrentVertex[I]) {
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
