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

#include <iostream>

#include <llvm-14/llvm/ADT/StringRef.h>
#include <llvm-14/llvm/BinaryFormat/Dwarf.h>
#include <llvm-14/llvm/IR/DebugInfoMetadata.h>

namespace psr {

DIBasedTypeHierarchy::DIBasedTypeHierarchy(const LLVMProjectIRDB &IRDB) {
  size_t Counter = 0;

  for (const auto *I : IRDB.getAllInstructions()) {
    I->getAllMetadata(MDs);
  }

  for (const auto &Node : MDs) {
    // std::cout << "MD-ID first loop: " << Node.second->getMetadataID()
    //           << std::endl;
  }

  for (const auto *F : IRDB.getAllFunctions()) {
    // TODO (max): Fix getting metadata nodes. Code below seems to not work
    F->getAllMetadata(MDs);
    // std::cout << "number of metadata nodes: " << MetadataNodes.size()
    //           << std::endl;
  }

  for (const auto &Node : MDs) {
    // std::cout << "MD-ID second loop: " << Node.second->getMetadataID()
    //           << std::endl;
    const auto &Kind = Node.second->getOperand(0).get();
  }

  // llvm::DIDerivedType::MDStringKind;
  // llvm::DIDerivedType::getBaseType();

  // std::cout << "number of Metadata nodes found: " << MDs.size() <<
  // std::endl;

  bool TestOnce = true;

  for (const auto &Node : MDs) {
    if (llvm::dyn_cast<llvm::DIDerivedType>(Node.second) && TestOnce &&
        Node.second->getNumOperands() > 1) {
      TestOnce = false;
      for (unsigned int I = 0; I < Node.second->getNumOperands(); I++) {
        const auto &Test = Node.second->getOperand(I)->getMetadataID();
      }
    }
    std::cout << "first: " << Node.first
              << " second: " << Node.second->getMetadataID() << std::endl;
    //   derived type, aka inheritance
    if (const llvm::DIDerivedType *DerivedType =
            llvm::dyn_cast<llvm::DIDerivedType>(
                Node.second->getOperand(0).get())) {

      std::cout << "made it" << std::endl;
      TypeToVertex.try_emplace(DerivedType, Counter++);
      VertexTypes.push_back(DerivedType);

      DerivedTypesOf[DerivedType->getMetadataID()].push_back(
          DerivedType->getBaseType()->getMetadataID());
      continue;
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
  OS << "Test Print of Operants…\n";
  for (const auto &Node : MDs) {
    // std::cout << "MD-ID second loop: " << Node.second->getMetadataID()
    //           << std::endl;
    Node.second->getOperand(0)->print(OS);
    OS << "\n";
  }
  OS << "\n";

  OS << "Size of Transitive Closure: " << TransitiveClosure.size() << "\n";
  OS << "Number of Vertices " << VertexTypes.size() << "\n";
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
