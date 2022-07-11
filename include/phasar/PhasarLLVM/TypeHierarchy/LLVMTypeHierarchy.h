/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ClassHierarchy.h
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMTYPEHIERARCHY_H_
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMTYPEHIERARCHY_H_

#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_traits.hpp"

#include "llvm/ADT/StringRef.h"

#include "gtest/gtest_prod.h"

#include "nlohmann/json.hpp"

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/PhasarLLVM/TypeHierarchy/TypeHierarchy.h"

namespace llvm {
class Module;
class StructType;
class Function;
class GlobalVariable;
} // namespace llvm

namespace psr {

class ProjectIRDB;
/**
 * 	@brief Owns the class hierarchy of the analyzed program.
 *
 * 	This class is responsible for constructing a inter-modular class
 * 	hierarchy graph based on the data from the %ProjectIRCompiledDB
 * 	and reconstructing the virtual method tables.
 */
class LLVMTypeHierarchy
    : public TypeHierarchy<const llvm::StructType *, const llvm::Function *> {
public:
  struct VertexProperties {
    VertexProperties() = default;
    VertexProperties(const llvm::StructType *Type);

    [[nodiscard]] std::string getTypeName() const;

    const llvm::StructType *Type = nullptr;
    std::optional<LLVMVFTable> VFT = std::nullopt;
    std::set<const llvm::StructType *> ReachableTypes;
  };

  /// Edges in the class hierarchy graph doesn't hold any additional
  /// information.
  struct EdgeProperties {
    EdgeProperties() = default;
  };

  /// Data structure holding the class hierarchy graph.
  using bidigraph_t =
      boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS,
                            VertexProperties, EdgeProperties>;

  /// The type for vertex representative objects.
  using vertex_t = boost::graph_traits<bidigraph_t>::vertex_descriptor;
  /// The type for edge representative objects.
  using edge_t = boost::graph_traits<bidigraph_t>::edge_descriptor;
  // Let us have some further handy typedefs.
  using vertex_iterator = boost::graph_traits<bidigraph_t>::vertex_iterator;
  using out_edge_iterator = boost::graph_traits<bidigraph_t>::out_edge_iterator;
  using in_edge_iterator = boost::graph_traits<bidigraph_t>::in_edge_iterator;

  static inline constexpr llvm::StringLiteral StructPrefix = "struct.";
  static inline constexpr llvm::StringLiteral ClassPrefix = "class.";
  static inline constexpr llvm::StringLiteral VTablePrefix = "_ZTV";
  static inline constexpr llvm::StringLiteral VTablePrefixDemang =
      "vtable for ";
  static inline constexpr llvm::StringLiteral TypeInfoPrefix = "_ZTI";
  static inline constexpr llvm::StringLiteral TypeInfoPrefixDemang =
      "typeinfo for ";
  static inline constexpr llvm::StringLiteral PureVirtualCallName =
      "__cxa_pure_virtual";

private:
  bidigraph_t TypeGraph;
  std::unordered_map<const llvm::StructType *, vertex_t> TypeVertexMap;
  // maps type names to the corresponding vtable
  std::unordered_map<const llvm::StructType *, LLVMVFTable> TypeVFTMap;
  // holds all modules that are included in the type hierarchy
  std::unordered_set<const llvm::Module *> VisitedModules;
  // helper map from clearname to type*
  std::unordered_map<std::string, const llvm::StructType *> ClearNameTypeMap;
  // map from clearname to type info variable
  std::unordered_map<std::string, const llvm::GlobalVariable *> ClearNameTIMap;
  // map from clearname to vtable variable
  std::unordered_map<std::string, const llvm::GlobalVariable *> ClearNameTVMap;

  static std::string removeStructOrClassPrefix(const llvm::StructType &T);

  static std::string removeStructOrClassPrefix(const std::string &TypeName);

  static std::string removeTypeInfoPrefix(std::string VarName);

  static std::string removeVTablePrefix(std::string VarName);

  static bool isTypeInfo(const std::string &VarName);

  static bool isVTable(const std::string &VarName);

  static bool isStruct(const llvm::StructType &T);

  static bool isStruct(llvm::StringRef TypeName);

  std::vector<const llvm::StructType *>
  getSubTypes(const llvm::Module &M, const llvm::StructType &Type);

  std::vector<const llvm::Function *>
  getVirtualFunctions(const llvm::Module &M, const llvm::StructType &Type);

  // FRIEND_TEST(VTableTest, SameTypeDifferentVTables);
  FRIEND_TEST(LTHTest, GraphConstruction);
  FRIEND_TEST(LTHTest, HandleLoadAndPrintOfNonEmptyGraph);

protected:
  void buildLLVMTypeHierarchy(const llvm::Module &M);

public:
  /**
   *  @brief Creates a LLVMStructTypeHierarchy based on the
   *         given ProjectIRCompiledDB.
   *  @param IRDB ProjectIRCompiledDB object.
   */
  LLVMTypeHierarchy(ProjectIRDB &IRDB);

  /**
   *  @brief Creates a LLVMStructTypeHierarchy based on the
   *         llvm::Module.
   *  @param M A llvm::Module.
   */
  LLVMTypeHierarchy(const llvm::Module &M);

  ~LLVMTypeHierarchy() override = default;

  /**
   * @brief Constructs the actual class hierarchy graph.
   * @param M LLVM module
   *
   * Extracts new information from the given module and adds new vertices
   * and edges accordingly to the type hierarchy graph.
   */
  void constructHierarchy(const llvm::Module &M);

  [[nodiscard]] inline bool
  hasType(const llvm::StructType *Type) const override {
    return TypeVertexMap.count(Type);
  }

  [[nodiscard]] inline bool
  isSubType(const llvm::StructType *Type,
            const llvm::StructType *SubType) override {
    auto ReachableTypes = getSubTypes(Type);
    return ReachableTypes.count(SubType);
  }

  std::set<const llvm::StructType *>
  getSubTypes(const llvm::StructType *Type) override;

  [[nodiscard]] inline bool
  isSuperType(const llvm::StructType *Type,
              const llvm::StructType *SuperType) override {
    return isSubType(SuperType, Type); // NOLINT
  }

  std::set<const llvm::StructType *>
  getSuperTypes(const llvm::StructType *Type) override;

  [[nodiscard]] const llvm::StructType *
  getType(std::string TypeName) const override;

  [[nodiscard]] std::set<const llvm::StructType *> getAllTypes() const override;

  [[nodiscard]] std::string
  getTypeName(const llvm::StructType *Type) const override;

  [[nodiscard]] bool hasVFTable(const llvm::StructType *Type) const override;

  [[nodiscard]] const LLVMVFTable *
  getVFTable(const llvm::StructType *Type) const override;

  [[nodiscard]] inline size_t size() const override {
    return boost::num_vertices(TypeGraph);
  };

  [[nodiscard]] inline bool empty() const override { return size() == 0; };

  void print(llvm::raw_ostream &OS = llvm::outs()) const override;

  [[nodiscard]] nlohmann::json getAsJson() const override;

  // void mergeWith(LLVMTypeHierarchy &Other);

  /**
   * 	@brief Prints the transitive closure of the class hierarchy graph.
   */
  void printTransitiveClosure(llvm::raw_ostream &OS = llvm::outs()) const;

  /**
   * 	@brief Prints the class hierarchy to an ostream in dot format.
   * 	@param an outputstream
   */
  void printAsDot(llvm::raw_ostream &OS = llvm::outs()) const;

  /**
   * @brief Prints the class hierarchy to an ostream in json format.
   * @param an outputstream
   */
  void printAsJson(llvm::raw_ostream &OS = llvm::outs()) const;

  // void printGraphAsDot(llvm::raw_ostream &out);

  // static bidigraph_t loadGraphFormDot(std::istream &in);
};

} // namespace psr

#endif
