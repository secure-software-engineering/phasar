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

#include "llvm/Support/ErrorHandling.h"

#include <llvm-14/llvm/ADT/SmallVector.h>
#include <llvm-14/llvm/IR/DebugInfoMetadata.h>
#include <llvm-14/llvm/IR/DerivedTypes.h>
#include <llvm-14/llvm/IR/GlobalVariable.h>
#include <llvm-14/llvm/IR/Metadata.h>
#include <llvm-14/llvm/Support/Casting.h>

namespace psr {
DIBasedTypeHierarchy::DIBasedTypeHierarchy(const LLVMProjectIRDB &IRDB) {
  for (const auto *F : IRDB.getAllFunctions()) {
    llvm::SmallVector<std::pair<unsigned, llvm::MDNode *>, 4> MDs;

    F->getAllMetadata(MDs);

    for (const auto &Node : MDs) {
      // basic type (like int for example)
      if (const llvm::DIBasicType *BasicType =
              llvm::dyn_cast<llvm::DIBasicType *>(Node)) {
        TypeToVertex.grow(llvm::Metadata::DIBasicTypeKind);
        VertexTypes.emplace_back(llvm::Metadata::DIBasicTypeKind);
        continue;
      }

      // composite type (like struct or class)
      if (const llvm::DICompositeType *CompositeType =
              llvm::dyn_cast<llvm::DICompositeType *>(Node)) {
        TypeToVertex.grow(llvm::Metadata::DICompositeTypeKind);
        VertexTypes.emplace_back(llvm::Metadata::DICompositeTypeKind);

        // determine how many variables the composite type has
        // TODO (max): Check how a composite type in a composite type will be
        // handled
        llvm::SmallVector<size_t, 6> SubTypes;
        for (const auto &Element : CompositeType->getElements()) {
          SubTypes.push_back(Element->getMetadataID());
        }

        DerivedTypesOf[CompositeType->getMetadataID()] = SubTypes;
        continue;
      }

      // derived type (like a pointer for example)
      if (const llvm::DIDerivedType *DerivedType =
              llvm::dyn_cast<llvm::DIDerivedType *>(Node)) {
        TypeToVertex.grow(llvm::Metadata::DIDerivedTypeKind);
        VertexTypes.emplace_back(llvm::Metadata::DIDerivedTypeKind);
        continue;
      }

      // string type
      if (const llvm::DIStringType *StringType =
              llvm::dyn_cast<llvm::DIStringType *>(Node)) {
        TypeToVertex.grow(llvm::Metadata::DIStringTypeKind);
        VertexTypes.emplace_back(llvm::Metadata::DIStringTypeKind);
        continue;
      }

      // global type
      if (const llvm::DIGlobalVariable *GlobalType =
              llvm::dyn_cast<llvm::DIGlobalVariable *>(Node)) {
        TypeToVertex.grow(llvm::Metadata::DIGlobalVariableKind);
        VertexTypes.emplace_back(llvm::Metadata::DIGlobalVariableKind);
        continue;
      }
    }
  }
}

[[nodiscard]] bool DIBasedTypeHierarchy::isSubType(ClassType Type,
                                                   ClassType SubType) {
  /// TODO: implement
  // You may want to do a graph search based on DerivedTypesOf

  // find index of super type
  int IndexOfType = -1;

  for (int I = 0; I < DerivedTypesOf.size(); I++) {
    if (DerivedTypesOf[I][0] == Type->getMetadataID()) {
      IndexOfType = I;
    }
  }

  // if the super type hasn't been found, return false
  if (IndexOfType == -1) {
    return false;
  }

  // go over all sub types of type and check if the sub type of interest is
  // present
  for (const auto &Current : DerivedTypesOf[Type->getMetadataID()]) {
    if (Current == SubType->getMetadataID()) {
      return true;
    }
  }

  return false;
  // llvm::report_fatal_error("Not implemented");
}

[[nodiscard]] auto DIBasedTypeHierarchy::getSubTypes(ClassType Type)
    -> std::set<ClassType> {
  /// TODO: implement
  // You may want to do a graph search based on DerivedTypesOf

  // find index of super type
  int IndexOfType = -1;

  for (unsigned int I = 0; I < DerivedTypesOf.size(); I++) {
    if (DerivedTypesOf[I][0] == Type->getMetadataID()) {
      IndexOfType = I;
    }
  }

  // if the super type hasn't been found, return an empty set
  if (IndexOfType == -1) {
    return {};
  }

  // return all sub types
  std::set<ClassType> SubTypes = {};
  for (unsigned long I : DerivedTypesOf[IndexOfType]) {
    SubTypes.insert(VertexTypes[I]);
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
  /// TODO: implement
  // Maybe take a look at Type->isVirtual()

  // Can't we just return Type->isVirtual() here? (max)
  return Type->isVirtual();

  // auto test = Type->getFlags();

  // llvm::report_fatal_error("Not implemented");
}

[[nodiscard]] auto DIBasedTypeHierarchy::getVFTable(ClassType Type) const
    -> const VFTable<f_t> * {
  /// TODO: implement
  // Use the VTables deque here; either you have that pre-computed, or you
  // create it on demand

  // Problem: getting VFTables from Metadata nodes seems to be not possible?
  // Therefore, creating VTables seems not possible aswell
  // I will search for a solution though

  // return VTables.at(Type->getMetadataID());

  llvm::report_fatal_error("Not implemented");
}

void DIBasedTypeHierarchy::print(llvm::raw_ostream &OS) const {
  /// TODO: implement
  OS << "Type Hierarchy:\n";

  for (const auto &CurrentVertex : VertexTypes) {
    OS << CurrentVertex->getName();

    if (!DerivedTypesOf[CurrentVertex->getMetadataID()].empty()) {
      for (const auto &CurrentDerived :
           DerivedTypesOf[CurrentVertex->getMetadataID()]) {
        OS << VertexTypes[CurrentDerived];
      }
    }
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
