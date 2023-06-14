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
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

namespace psr {

DIBasedTypeHierarchy::DIBasedTypeHierarchy(const LLVMProjectIRDB &IRDB) {
  size_t Counter = 0;

  for (const auto *F : IRDB.getAllFunctions()) {

    llvm::SmallVector<std::pair<unsigned, llvm::MDNode *>, 4> MDs;
    F->getAllMetadata(MDs);

    for (const auto &Node : MDs) {
      // composite type (like struct or class)
      if (const llvm::DICompositeType *CompositeType =
              llvm::dyn_cast<llvm::DICompositeType>(Node.second)) {
        TypeToVertex.try_emplace(CompositeType, Counter++);
        VertexTypes.push_back(CompositeType);

        // determine how many variables the composite type has
        // BUG; INHERITANCE nicht member variablen typen
        llvm::SmallVector<size_t, 6> SubTypes;
        for (const auto &Element : CompositeType->getElements()) {
          SubTypes.push_back(Element->getMetadataID());
        }

        DerivedTypesOf[CompositeType->getMetadataID()] = SubTypes;
        continue;
      }
    }
  }

  // Initialize the transitive closure matrix with all as false
  std::vector<bool> InitVector(VertexTypes.size(), false);

  for (size_t I = 0; I < VertexTypes.size(); I++) {
    TransitiveClosure.push_back(InitVector);
  }

  // Add direct edges
  unsigned int CurrentIndex = 0;
  for (const auto &Edges : DerivedTypesOf) {
    for (const auto &Current : Edges) {
      TransitiveClosure.at(CurrentIndex).at(Current) = true;
    }
    CurrentIndex++;
  }

  // Add transitive edges
  for (auto &Row : TransitiveClosure) {
    size_t Index = 0;
    for (auto Edge : Row) {
      if (Edge) {
        addSubtypes(Row, Index);
      }
      Index++;
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
  OS << "Type Hierarchy:\n";

  size_t VertexIndex = 0;
  size_t EdgeIndex = 0;
  for (const auto &CurrentVertex : TransitiveClosure) {
    OS << VertexTypes.at(VertexIndex)->getName() << "\n";
    for (const bool CurrentEdge : CurrentVertex) {
      if (EdgeIndex != VertexIndex && CurrentEdge) {
        OS << "--> " << VertexTypes[EdgeIndex] << "\n";
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

void DIBasedTypeHierarchy::addSubtypes(std::vector<bool> &Row,
                                       size_t OtherRowIndex) {
  size_t Index = 0;
  for (auto Edge : TransitiveClosure[OtherRowIndex]) {
    if (Edge) {
      Row[Index] = true;
      addSubtypes(Row, Index);
    }

    Index++;
  }
}
} // namespace psr
