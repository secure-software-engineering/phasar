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

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchyData.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/TypeHierarchy/TypeHierarchy.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_traits.hpp"

#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace llvm {
class Module;
class StructType;
class Function;
class GlobalVariable;
} // namespace llvm

namespace psr {

class LLVMProjectIRDB;
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

  std::vector<const llvm::StructType *>
  getSubTypes(const llvm::Module &M, const llvm::StructType &Type) const;

  std::vector<const llvm::Function *>
  getVirtualFunctions(const llvm::Module &M, const llvm::StructType &Type);

protected:
  void buildLLVMTypeHierarchy(const llvm::Module &M);

public:
  static bool isTypeInfo(llvm::StringRef VarName);
  static bool isVTable(llvm::StringRef VarName);
  static bool isStruct(const llvm::StructType &T);
  static bool isStruct(llvm::StringRef TypeName);

  static std::string removeStructOrClassPrefix(const llvm::StructType &T);
  static std::string removeStructOrClassPrefix(llvm::StringRef TypeName);
  static std::string removeTypeInfoPrefix(llvm::StringRef VarName);
  static std::string removeVTablePrefix(llvm::StringRef VarName);

  /**
   *  @brief Creates a LLVMStructTypeHierarchy based on the
   *         given ProjectIRCompiledDB.
   *  @param IRDB ProjectIRCompiledDB object.
   */
  LLVMTypeHierarchy(const LLVMProjectIRDB &IRDB);
  LLVMTypeHierarchy(const LLVMProjectIRDB &IRDB,
                    const LLVMTypeHierarchyData &SerializedData);

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
            const llvm::StructType *SubType) const override {
    auto ReachableTypes = getSubTypes(Type);
    return ReachableTypes.count(SubType);
  }

  std::set<const llvm::StructType *>
  getSubTypes(const llvm::StructType *Type) const override;

  [[nodiscard]] const llvm::StructType *
  getType(llvm::StringRef TypeName) const override;

  [[nodiscard]] std::vector<const llvm::StructType *>
  getAllTypes() const override;

  [[nodiscard]] llvm::StringRef
  getTypeName(const llvm::StructType *Type) const override;

  [[nodiscard]] size_t size() const noexcept override {
    return boost::num_vertices(TypeGraph);
  };

  [[nodiscard]] bool empty() const noexcept override {
    return boost::num_vertices(TypeGraph) == 0;
  };

  void print(llvm::raw_ostream &OS = llvm::outs()) const override;

  [[nodiscard]] [[deprecated(
      "Please use printAsJson() instead")]] nlohmann::json
  getAsJson() const override;

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
  void printAsJson(llvm::raw_ostream &OS = llvm::outs()) const override;

  // void printGraphAsDot(llvm::raw_ostream &out);

  // static bidigraph_t loadGraphFormDot(std::istream &in);
};

} // namespace psr

#endif
