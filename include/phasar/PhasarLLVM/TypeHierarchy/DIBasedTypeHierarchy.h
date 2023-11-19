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

#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"

#include <deque>

namespace psr {
class LLVMProjectIRDB;

class DIBasedTypeHierarchy
    : public TypeHierarchy<const llvm::DIType *, const llvm::Function *> {
public:
  using ClassType = const llvm::DIType *;
  using f_t = const llvm::Function *;

  explicit DIBasedTypeHierarchy(const LLVMProjectIRDB &IRDB);
  ~DIBasedTypeHierarchy() override = default;

  [[nodiscard]] bool hasType(ClassType Type) const override {
    return TypeToVertex.count(Type);
  }

  [[nodiscard]] bool isSubType(ClassType Type, ClassType SubType) override {
    return llvm::is_contained(subTypesOf(Type), SubType);
  }

  [[nodiscard]] std::set<ClassType> getSubTypes(ClassType Type) override {
    const auto &Range = subTypesOf(Type);
    return {Range.begin(), Range.end()};
  }

  /// A more efficient version of getSubTypes()
  [[nodiscard]] llvm::iterator_range<const ClassType *>
  subTypesOf(ClassType Ty) const noexcept;

  [[nodiscard]] bool isSuperType(ClassType Type, ClassType SuperType) override;

  /// Not supported yet
  [[nodiscard]] std::set<ClassType> getSuperTypes(ClassType Type) override;

  [[nodiscard]] ClassType
  getType(std::string TypeName) const noexcept override {
    return NameToType.lookup(TypeName);
  }

  [[nodiscard]] std::set<ClassType> getAllTypes() const override {
    return {VertexTypes.begin(), VertexTypes.end()};
  }

  [[nodiscard]] const auto &getAllVTables() const noexcept { return VTables; }

  [[nodiscard]] std::string getTypeName(ClassType Type) const override {
    return Type->getName().str();
  }

  [[nodiscard]] bool hasVFTable(ClassType Type) const override;

  [[nodiscard]] const VFTable<f_t> *getVFTable(ClassType Type) const override {
    auto It = TypeToVertex.find(Type);
    if (It == TypeToVertex.end()) {
      return nullptr;
    }
    return &VTables[It->second];
  }

  [[nodiscard]] size_t size() const override { return VertexTypes.size(); }

  [[nodiscard]] bool empty() const override { return VertexTypes.empty(); }

  void print(llvm::raw_ostream &OS = llvm::outs()) const override;

  /**
   * 	@brief Prints the class hierarchy to an ostream in dot format.
   * 	@param OS outputstream
   */
  void printAsDot(llvm::raw_ostream &OS = llvm::outs()) const;

  [[nodiscard]] nlohmann::json getAsJson() const override;

private:
  [[nodiscard]] llvm::iterator_range<const ClassType *>
  subTypesOf(size_t TypeIdx) const noexcept;

  // ---

  llvm::StringMap<ClassType> NameToType;
  // Map each type to an integer index that is used by VertexTypes and
  // DerivedTypesOf.
  // Note: all the below arrays should always have the same size (except for
  // Hierarchy)!
  llvm::DenseMap<ClassType, size_t> TypeToVertex;
  // The class types we care about ("VertexProperties")
  std::vector<const llvm::DICompositeType *> VertexTypes;
  std::vector<std::pair<uint32_t, uint32_t>> TransitiveDerivedIndex;
  // The inheritance graph linearized as-if constructed by L2R pre-order
  // traversal from the roots. Allows efficient access to the transitive closure
  // without ever storing it explicitly. This only works, because the type-graph
  // is known to never contain loops
  std::vector<ClassType> Hierarchy;

  // The VTables of the polymorphic types in the TH. default-constructed if not
  // exists
  std::deque<LLVMVFTable> VTables;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_TYPEHIERARCHY_DIBASEDTYPEHIERARCHY_H
