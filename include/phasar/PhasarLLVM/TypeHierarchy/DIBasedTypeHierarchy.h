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

#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchyData.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/TypeHierarchy/TypeHierarchy.h"

#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/Casting.h"

#include <deque>

namespace psr {
class LLVMProjectIRDB;

class DIBasedTypeHierarchy
    : public TypeHierarchy<const llvm::DIType *, const llvm::Function *> {
public:
  using ClassType = const llvm::DIType *;
  using f_t = const llvm::Function *;

  static inline constexpr llvm::StringLiteral StructPrefix = "struct.";
  static inline constexpr llvm::StringLiteral ClassPrefix = "class.";
  static inline constexpr llvm::StringLiteral VTablePrefix = "_ZTV";
  static inline constexpr llvm::StringLiteral VTablePrefixDemang =
      "vtable for ";
  static inline constexpr llvm::StringLiteral PureVirtualCallName =
      "__cxa_pure_virtual";

  explicit DIBasedTypeHierarchy(const LLVMProjectIRDB &IRDB);
  explicit DIBasedTypeHierarchy(const LLVMProjectIRDB *IRDB,
                                const DIBasedTypeHierarchyData &SerializedData);
  ~DIBasedTypeHierarchy() override = default;

  static bool isVTable(llvm::StringRef VarName);
  static std::string removeVTablePrefix(llvm::StringRef VarName);

  [[nodiscard]] bool hasType(ClassType Type) const override {
    return TypeToVertex.count(Type);
  }

  [[nodiscard]] bool isSubType(ClassType Type,
                               ClassType SubType) const override {
    return llvm::is_contained(subTypesOf(Type), SubType);
  }

  [[nodiscard]] std::set<ClassType> getSubTypes(ClassType Type) const override {
    const auto &Range = subTypesOf(Type);
    return {Range.begin(), Range.end()};
  }

  /// A more efficient version of getSubTypes()
  [[nodiscard]] llvm::iterator_range<const ClassType *>
  subTypesOf(ClassType Ty) const noexcept;

  [[nodiscard]] ClassType
  getType(llvm::StringRef TypeName) const noexcept override {
    return NameToType.lookup(TypeName);
  }

  [[nodiscard]] std::vector<ClassType> getAllTypes() const override {
    return {VertexTypes.begin(), VertexTypes.end()};
  }

  [[nodiscard]] const auto &getAllVTables() const noexcept { return VTables; }

  [[nodiscard]] llvm::StringRef getTypeName(ClassType Type) const override {
    if (const auto *CompTy = llvm::dyn_cast<llvm::DICompositeType>(Type)) {
      auto Ident = CompTy->getIdentifier();
      return Ident.empty() ? CompTy->getName() : Ident;
    }
    return Type->getName();
  }

  [[nodiscard]] size_t size() const noexcept override {
    return VertexTypes.size();
  }
  [[nodiscard]] bool empty() const noexcept override {
    return VertexTypes.empty();
  }

  void print(llvm::raw_ostream &OS = llvm::outs()) const override;

  /**
   * 	@brief Prints the class hierarchy to an ostream in dot format.
   * 	@param OS outputstream
   */
  void printAsDot(llvm::raw_ostream &OS = llvm::outs()) const;

  [[nodiscard]] [[deprecated(
      "Please use printAsJson() instead")]] nlohmann::json
  getAsJson() const override;

  /**
   * @brief Prints the class hierarchy to an ostream in json format.
   * @param an outputstream
   */
  void printAsJson(llvm::raw_ostream &OS = llvm::outs()) const override;

private:
  [[nodiscard]] DIBasedTypeHierarchyData getTypeHierarchyData() const;
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
