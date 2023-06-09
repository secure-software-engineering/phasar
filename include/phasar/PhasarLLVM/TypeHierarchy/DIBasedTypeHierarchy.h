/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_DIBASEDTYPEHIERARCHY_H
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_DIBASEDTYPEHIERARCHY_H

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/TypeHierarchy/TypeHierarchy.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/DebugInfoMetadata.h"

#include <deque>

namespace psr {
class LLVMProjectIRDB;

class DIBasedTypeHierarchy
    : public TypeHierarchy<const llvm::DIType *, const llvm::Function *> {
public:
  using ClassType = const llvm::DIType *;
  using f_t = const llvm::Function *;

  DIBasedTypeHierarchy(const LLVMProjectIRDB &IRDB);
  ~DIBasedTypeHierarchy() override = default;

  [[nodiscard]] bool hasType([[maybe_unused]] ClassType Type) const override {
    return TypeToVertex.count(Type);
  }

  [[nodiscard]] bool isSubType(ClassType Type, ClassType SubType) override;

  [[nodiscard]] std::set<ClassType> getSubTypes(ClassType Type) override;

  [[nodiscard]] bool isSuperType(ClassType Type, ClassType SuperType) override;

  [[nodiscard]] std::set<ClassType> getSuperTypes(ClassType Type) override;

  [[nodiscard]] ClassType getType(llvm::StringRef TypeName) const noexcept {
    return NameToType.lookup(TypeName);
  }

  [[nodiscard]] std::set<ClassType> getAllTypes() const override {
    return {VertexTypes.begin(), VertexTypes.end()};
  }

  [[nodiscard]] std::string getTypeName(ClassType Type) const override {
    /// TODO: Check: is this correct?
    return Type->getName().str();
  }

  [[nodiscard]] bool hasVFTable(ClassType Type) const override;

  [[nodiscard]] const VFTable<f_t> *getVFTable(ClassType Type) const override;

  [[nodiscard]] size_t size() const override { return VertexTypes.size(); }

  [[nodiscard]] bool empty() const override { return VertexTypes.empty(); }

  void print(llvm::raw_ostream &OS = llvm::outs()) const override;

  [[nodiscard]] nlohmann::json getAsJson() const override;

private:
  [[nodiscard]] ClassType getType(std::string TypeName) const override {
    return NameToType.lookup(TypeName);
  }

  llvm::StringMap<ClassType> NameToType;

  // Map each type to an integer index that is used by VertexTypes and
  // DerivedTypesOf.
  // Note: all the below arrays should always have the same size!
  llvm::DenseMap<ClassType, size_t> TypeToVertex;
  // The class types we care about ("VertexProperties")
  std::vector<ClassType> VertexTypes;
  // The type-graph edges ("Adjacency List").
  // DerivedTypesOf[TypeToVertex.lookup(Ty)] gives the indices of the direct
  // subclasses of type T
  std::vector<llvm::SmallVector<size_t>> DerivedTypesOf;
  // The VTables of the polymorphic types in the TH. default-constructed if not
  // exists
  std::deque<LLVMVFTable> VTables;
  // Transitive closure implemented as a matrix
  // Example:
  // Graph:
  // (1) -> (3)
  //  ^
  //  |
  // (2)
  // Transitive closure:
  // 1 0 1
  // 1 1 1
  // 0 0 1
  std::vector<std::vector<bool>> TransitiveClosure;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_TYPEHIERARCHY_DIBASEDTYPEHIERARCHY_H
