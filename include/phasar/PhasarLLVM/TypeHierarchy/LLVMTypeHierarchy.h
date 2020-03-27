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

#include <iostream>
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
    const llvm::StructType *Type = nullptr;
    std::optional<LLVMVFTable> VFT;
    std::set<const llvm::StructType *> ReachableTypes;
    std::string getTypeName() const;
  };

  /// Edges in the class hierarchy graph doesn't hold any additional
  /// information.
  struct EdgeProperties {
    EdgeProperties() = default;
  };

  /// Data structure holding the class hierarchy graph.
  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS,
                                VertexProperties, EdgeProperties>
      bidigraph_t;

  /// The type for vertex representative objects.
  typedef boost::graph_traits<bidigraph_t>::vertex_descriptor vertex_t;
  /// The type for edge representative objects.
  typedef boost::graph_traits<bidigraph_t>::edge_descriptor edge_t;
  // Let us have some further handy typedefs.
  typedef boost::graph_traits<bidigraph_t>::vertex_iterator vertex_iterator;
  typedef boost::graph_traits<bidigraph_t>::out_edge_iterator out_edge_iterator;
  typedef boost::graph_traits<bidigraph_t>::in_edge_iterator in_edge_iterator;

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

  static const std::string StructPrefix;

  static const std::string ClassPrefix;

  static const std::string VTablePrefix;

  static const std::string VTablePrefixDemang;

  static const std::string TypeInfoPrefix;

  static const std::string TypeInfoPrefixDemang;

  std::string removeStructOrClassPrefix(const llvm::StructType &T);

  std::string removeStructOrClassPrefix(const std::string &TypeName);

  std::string removeTypeInfoPrefix(std::string VarName);

  std::string removeVTablePrefix(std::string VarName);

  bool isTypeInfo(std::string VarName);

  bool isVTable(std::string VarName);

  bool isStruct(const llvm::StructType &T);

  bool isStruct(llvm::StringRef TypeName);

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

  ~LLVMTypeHierarchy() = default;

  /**
   * @brief Constructs the actual class hierarchy graph.
   * @param M LLVM module
   *
   * Extracts new information from the given module and adds new vertices
   * and edges accordingly to the type hierarchy graph.
   */
  void constructHierarchy(const llvm::Module &M);

  bool hasType(const llvm::StructType *Type) const override;

  bool isSubType(const llvm::StructType *Type,
                 const llvm::StructType *SubType) override;

  std::set<const llvm::StructType *>
  getSubTypes(const llvm::StructType *Type) override;

  bool isSuperType(const llvm::StructType *Type,
                   const llvm::StructType *SuperType) override;

  std::set<const llvm::StructType *>
  getSuperTypes(const llvm::StructType *Type) override;

  const llvm::StructType *getType(std::string TypeName) const override;

  std::set<const llvm::StructType *> getAllTypes() const override;

  std::string getTypeName(const llvm::StructType *Type) const override;

  bool hasVFTable(const llvm::StructType *Type) const override;

  const LLVMVFTable *getVFTable(const llvm::StructType *Type) const override;

  size_t size() const override;

  bool empty() const override;

  void print(std::ostream &OS = std::cout) const override;

  nlohmann::json getAsJson() const override;

  // void mergeWith(LLVMTypeHierarchy &Other);

  /**
   * 	@brief Prints the transitive closure of the class hierarchy graph.
   */
  void printTransitiveClosure(std::ostream &OS = std::cout) const;

  // provide a VertexPropertyWrite to tell boost how to write a vertex
  class TypeHierarchyVertexWriter {
  public:
    TypeHierarchyVertexWriter(const bidigraph_t &TyGraph) : TyGraph(TyGraph) {}
    template <class VertexOrEdge>
    void operator()(std::ostream &out, const VertexOrEdge &v) const {
      out << "[label=\"" << TyGraph[v].getTypeName() << "\"]";
    }

  private:
    const bidigraph_t &TyGraph;
  };

  // a function to conveniently create this writer
  TypeHierarchyVertexWriter
  makeTypeHierarchyVertexWriter(const bidigraph_t &TyGraph) const {
    return TypeHierarchyVertexWriter(TyGraph);
  }

  /**
   * 	@brief Prints the class hierarchy to an ostream in dot format.
   * 	@param an outputstream
   */
  void printAsDot(std::ostream &OS = std::cout) const;

  /**
   * @brief Prints the class hierarchy to an ostream in json format.
   * @param an outputstream
   */
  void printAsJson(std::ostream &OS = std::cout) const;

  // void printGraphAsDot(std::ostream &out);

  // static bidigraph_t loadGraphFormDot(std::istream &in);
};

} // namespace psr

#endif
