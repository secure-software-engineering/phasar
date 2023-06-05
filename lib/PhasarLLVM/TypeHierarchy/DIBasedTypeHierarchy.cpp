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

#include "llvm/Support/ErrorHandling.h"

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
      // TODO (max): create edges in the graph

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

  llvm::report_fatal_error("Not implemented");
}

[[nodiscard]] auto DIBasedTypeHierarchy::getSubTypes(ClassType Type)
    -> std::set<ClassType> {
  /// TODO: implement
  // You may want to do a graph search based on DerivedTypesOf
  llvm::report_fatal_error("Not implemented");
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
  llvm::report_fatal_error("Not implemented");
}

void DIBasedTypeHierarchy::print(llvm::raw_ostream &OS) const {
  /// TODO: implement
  llvm::report_fatal_error("Not implemented");
}

[[nodiscard]] nlohmann::json DIBasedTypeHierarchy::getAsJson() const {
  /// TODO: implement
  llvm::report_fatal_error("Not implemented");
}
} // namespace psr
