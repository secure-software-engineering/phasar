/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ClassHierarchy.cpp
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>

#include "boost/core/demangle.hpp"
#include "boost/log/sources/record_ostream.hpp"

#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/graphviz.hpp"
#include "boost/graph/transitive_closure.hpp"
#include "boost/property_map/dynamic_property_map.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/GraphExtensions.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Utilities.h"

using namespace psr;
using namespace std;

namespace psr {

const std::string LLVMTypeHierarchy::StructPrefix = "struct.";

const std::string LLVMTypeHierarchy::ClassPrefix = "class.";

const std::string LLVMTypeHierarchy::VTablePrefix = "_ZTV";

const std::string LLVMTypeHierarchy::VTablePrefixDemang = "vtable for ";

const std::string LLVMTypeHierarchy::TypeInfoPrefix = "_ZTI";

const std::string LLVMTypeHierarchy::TypeInfoPrefixDemang = "typeinfo for ";

LLVMTypeHierarchy::VertexProperties::VertexProperties(
    const llvm::StructType *Type)
    : Type(Type), ReachableTypes({Type}) {}

std::string LLVMTypeHierarchy::VertexProperties::getTypeName() const {
  return Type->getStructName().str();
}

LLVMTypeHierarchy::LLVMTypeHierarchy(ProjectIRDB &IRDB) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO) << "Construct type hierarchy");
  for (auto *M : IRDB.getAllModules()) {
    buildLLVMTypeHierarchy(*M);
  }
}

LLVMTypeHierarchy::LLVMTypeHierarchy(const llvm::Module &M) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO) << "Construct type hierarchy");
  buildLLVMTypeHierarchy(M);
}

std::string
LLVMTypeHierarchy::removeStructOrClassPrefix(const llvm::StructType &T) {
  return removeStructOrClassPrefix(T.getName().str());
}

std::string
LLVMTypeHierarchy::removeStructOrClassPrefix(const std::string &TypeName) {
  llvm::StringRef SR(TypeName);
  if (SR.startswith(StructPrefix)) {
    return SR.drop_front(StructPrefix.size()).str();
  }
  if (SR.startswith(ClassPrefix)) {
    return SR.drop_front(ClassPrefix.size()).str();
  }
  return TypeName;
}

std::string LLVMTypeHierarchy::removeTypeInfoPrefix(std::string VarName) {
  llvm::StringRef SR(VarName);
  if (SR.startswith(TypeInfoPrefixDemang)) {
    return SR.drop_front(TypeInfoPrefixDemang.size()).str();
  }
  if (SR.startswith(TypeInfoPrefix)) {
    return SR.drop_front(TypeInfoPrefix.size()).str();
  }
  return VarName;
}

std::string LLVMTypeHierarchy::removeVTablePrefix(std::string VarName) {
  llvm::StringRef SR(VarName);
  if (SR.startswith(VTablePrefixDemang)) {
    return SR.drop_front(VTablePrefixDemang.size()).str();
  }
  if (SR.startswith(VTablePrefix)) {
    return SR.drop_front(VTablePrefix.size()).str();
  }
  return VarName;
}

bool LLVMTypeHierarchy::isTypeInfo(const std::string &VarName) {
  auto Demang = boost::core::demangle(VarName.c_str());
  return llvm::StringRef(Demang).startswith(TypeInfoPrefixDemang);
}

bool LLVMTypeHierarchy::isVTable(const std::string &VarName) {
  auto Demang = boost::core::demangle(VarName.c_str());
  return llvm::StringRef(Demang).startswith(VTablePrefixDemang);
}

bool LLVMTypeHierarchy::isStruct(const llvm::StructType &T) {
  return isStruct(T.getName());
}

bool LLVMTypeHierarchy::isStruct(llvm::StringRef TypeName) {
  return TypeName.startswith(StructPrefix);
}

void LLVMTypeHierarchy::buildLLVMTypeHierarchy(const llvm::Module &M) {
  // build the hierarchy for the module
  constructHierarchy(M);
  // cache the reachable types
  bidigraph_t TC;
  boost::transitive_closure(TypeGraph, TC);
  for (auto V : boost::make_iterator_range(boost::vertices(TypeGraph))) {
    for (auto OE : boost::make_iterator_range(boost::out_edges(V, TC))) {
      auto Target = boost::target(OE, TC);
      TypeGraph[V].ReachableTypes.insert(TypeGraph[Target].Type);
    }
  }
}

std::vector<const llvm::StructType *>
LLVMTypeHierarchy::getSubTypes(const llvm::Module & /*M*/,
                               const llvm::StructType &Type) {
  // find corresponding type info variable
  std::vector<const llvm::StructType *> SubTypes;
  std::string ClearName = removeStructOrClassPrefix(Type);
  if (const auto *TI = ClearNameTIMap[ClearName]) {
    if (!TI->hasInitializer()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << ClearName << " does not have initializer");
      return SubTypes;
    }
    if (const auto *I =
            llvm::dyn_cast<llvm::ConstantStruct>(TI->getInitializer())) {
      for (const auto &Op : I->operands()) {
        if (auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(Op)) {
          std::unique_ptr<llvm::Instruction, decltype(&deleteValue)> AsI(
              CE->getAsInstruction(), &deleteValue);
          if (auto *BC = llvm::dyn_cast<llvm::BitCastInst>(AsI.get())) {
            if (BC->getOperand(0)->hasName()) {
              auto Name = BC->getOperand(0)->getName();
              if (Name.find(TypeInfoPrefix) != llvm::StringRef::npos) {
                auto ClearName = removeTypeInfoPrefix(
                    boost::core::demangle(Name.str().c_str()));
                if (const auto *Type = ClearNameTypeMap[ClearName]) {
                  SubTypes.push_back(Type);
                }
              }
            }
          }
        }
      }
    }
  }
  return SubTypes;
}

std::vector<const llvm::Function *>
LLVMTypeHierarchy::getVirtualFunctions(const llvm::Module &M,
                                       const llvm::StructType &Type) {
  auto ClearName = removeStructOrClassPrefix(Type.getName().str());
  std::vector<const llvm::Function *> VFS;
  if (const auto *TV = ClearNameTVMap[ClearName]) {
    if (const auto *TI = llvm::dyn_cast<llvm::GlobalVariable>(TV)) {
      if (!TI->hasInitializer()) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << ClearName << " does not have initializer");
        return VFS;
      }
      if (const auto *I =
              llvm::dyn_cast<llvm::ConstantStruct>(TI->getInitializer())) {
        for (const auto &Op : I->operands()) {
          if (auto *CA = llvm::dyn_cast<llvm::ConstantArray>(Op)) {
            for (auto &COp : CA->operands()) {
              if (auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(COp)) {
                std::unique_ptr<llvm::Instruction, decltype(&deleteValue)> AsI(
                    CE->getAsInstruction(), &deleteValue);
                if (auto *BC = llvm::dyn_cast<llvm::BitCastInst>(AsI.get())) {
                  // if the entry is a GlobalAlias, get its Aliasee
                  auto *ENTRY = BC->getOperand(0);
                  while (auto *GA = llvm::dyn_cast<llvm::GlobalAlias>(ENTRY)) {
                    ENTRY = GA->getAliasee();
                  }

                  if (ENTRY->hasName()) {
                    if (auto *F = M.getFunction(ENTRY->getName())) {
                      VFS.push_back(F);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return VFS;
}

void LLVMTypeHierarchy::constructHierarchy(const llvm::Module &M) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "Analyze types in module: " << M.getModuleIdentifier());
  // store analyzed module
  VisitedModules.insert(&M);
  auto StructTypes = M.getIdentifiedStructTypes();
  // build helper maps
  for (auto *StructType : StructTypes) {
    ClearNameTypeMap[removeStructOrClassPrefix(*StructType)] = StructType;
  }
  for (const auto &Global : M.globals()) {
    if (Global.hasName()) {
      if (isTypeInfo(Global.getName().str())) {
        auto Demang = boost::core::demangle(Global.getName().str().c_str());
        auto ClearName = removeTypeInfoPrefix(Demang);
        ClearNameTIMap[ClearName] = &Global;
      }
      if (isVTable(Global.getName().str())) {
        auto Demang = boost::core::demangle(Global.getName().str().c_str());
        auto ClearName = removeVTablePrefix(Demang);
        ClearNameTVMap[ClearName] = &Global;
      }
    }
  }
  // iterate struct types and add vertices
  for (auto *StructType : StructTypes) {
    if (!TypeVertexMap.count(StructType)) {
      auto Vertex = boost::add_vertex(TypeGraph);
      TypeVertexMap[StructType] = Vertex;
      TypeGraph[Vertex] = VertexProperties(StructType);
      TypeVFTMap[StructType] = getVirtualFunctions(M, *StructType);
    }
  }
  // construct the edges between a type and its subtypes
  for (auto *StructType : StructTypes) {
    // use type information to check if it is really a subtype
    auto SubTypes = getSubTypes(M, *StructType);
    for (const auto *SubType : SubTypes) {
      boost::add_edge(TypeVertexMap[SubType], TypeVertexMap[StructType],
                      TypeGraph);
    }
  }
}

std::set<const llvm::StructType *>
LLVMTypeHierarchy::getSubTypes(const llvm::StructType *Type) {
  if (TypeVertexMap.count(Type)) {
    return TypeGraph[TypeVertexMap[Type]].ReachableTypes;
  }
  return {};
}

std::set<const llvm::StructType *>
LLVMTypeHierarchy::getSuperTypes(const llvm::StructType * /*Type*/) {
  std::set<const llvm::StructType *> ReachableTypes;
  // TODO (philipp): what does this function do?
  return ReachableTypes;
}

const llvm::StructType *LLVMTypeHierarchy::getType(std::string TypeName) const {
  for (auto V : boost::make_iterator_range(boost::vertices(TypeGraph))) {
    if (TypeGraph[V].Type->getName() == TypeName) {
      return TypeGraph[V].Type;
    }
  }
  return nullptr;
}

std::set<const llvm::StructType *> LLVMTypeHierarchy::getAllTypes() const {
  std::set<const llvm::StructType *> Types;
  for (auto V : boost::make_iterator_range(boost::vertices(TypeGraph))) {
    Types.insert(TypeGraph[V].Type);
  }
  return Types;
}

std::string LLVMTypeHierarchy::getTypeName(const llvm::StructType *Type) const {
  return Type->getStructName().str();
}

bool LLVMTypeHierarchy::hasVFTable(const llvm::StructType *Type) const {
  if (TypeVFTMap.count(Type)) {
    return !TypeVFTMap.at(Type).empty();
  }
  return false;
}

const LLVMVFTable *
LLVMTypeHierarchy::getVFTable(const llvm::StructType *Type) const {
  if (TypeVFTMap.count(Type)) {
    return &TypeVFTMap.at(Type);
  }
  return nullptr;
}

void LLVMTypeHierarchy::print(std::ostream &OS) const {
  OS << "Type Hierarchy:\n";
  vertex_iterator UI;

  vertex_iterator UIEnd;
  for (boost::tie(UI, UIEnd) = boost::vertices(TypeGraph); UI != UIEnd; ++UI) {
    OS << TypeGraph[*UI].getTypeName() << " --> ";
    out_edge_iterator EI;

    out_edge_iterator EIEnd;
    for (boost::tie(EI, EIEnd) = boost::out_edges(*UI, TypeGraph); EI != EIEnd;
         ++EI) {
      OS << TypeGraph[target(*EI, TypeGraph)].getTypeName() << " ";
    }
    OS << '\n';
  }
  OS << "VFTables:\n";
  for (const auto &[Ty, VFT] : TypeVFTMap) {
    OS << "Virtual function table for: " << Ty->getName().str() << '\n';
    for (const auto *F : VFT) {
      OS << "\t-" << F->getName().str() << '\n';
    }
  }
}

nlohmann::json LLVMTypeHierarchy::getAsJson() const {
  nlohmann::json J;
  vertex_iterator VIv;

  vertex_iterator VIvEnd;
  out_edge_iterator EI;

  out_edge_iterator EIEnd;
  // iterate all graph vertices
  for (boost::tie(VIv, VIvEnd) = boost::vertices(TypeGraph); VIv != VIvEnd;
       ++VIv) {
    J[PhasarConfig::JsonTypeHierarchyID()][TypeGraph[*VIv].getTypeName()];
    // iterate all out edges of vertex vi_v
    for (boost::tie(EI, EIEnd) = boost::out_edges(*VIv, TypeGraph); EI != EIEnd;
         ++EI) {
      J[PhasarConfig::JsonTypeHierarchyID()][TypeGraph[*VIv].getTypeName()] +=
          TypeGraph[boost::target(*EI, TypeGraph)].getTypeName();
    }
  }
  return J;
}

// void LLVMTypeHierarchy::mergeWith(LLVMTypeHierarchy &Other) {
//   cout << "LLVMTypeHierarchy::mergeWith()" << endl;
//   boost::copy_graph(Other.TypeGraph, TypeGraph); // G += H;
//   // build the contractions
//   vector<pair<vertex_t, vertex_t>> contractions;
//   map<string, vertex_t> observed;
//   for (auto V : boost::make_iterator_range(boost::vertices(TypeGraph))) {
//     if (observed.find(TypeGraph[V].name) != observed.end()) {
//       // check which one has the valid pointer and the knowledge of the
//       vtables if (TypeGraph[V].llvmtype) {
//         contractions.push_back(make_pair(observed[TypeGraph[V].name], V));
//       } else {
//         contractions.push_back(make_pair(V, observed[TypeGraph[V].name]));
//       }
//     } else {
//       observed[TypeGraph[V].name] = V;
//     }
//   }
//   cout << "contractions.size(): " << contractions.size() << '\n';
//   for (auto contraction : contractions) {
//     contract_vertices<bidigraph_t, vertex_t, EdgeProperties>(
//         contraction.first, contraction.second, TypeGraph);
//   }
//   // merge the vtables
//   TypeVFTMap.insert(Other.TypeVFTMap.begin(),
//   Other.TypeVFTMap.end());
//   // merge the modules analyzed
//   contained_modules.insert(Other.contained_modules.begin(),
//                            Other.contained_modules.end());
//   // reset the vertex mapping
//   TypeVertexMap.clear();
//   for (auto V : boost::make_iterator_range(boost::vertices(TypeGraph))) {
//     TypeVertexMap[TypeGraph[V].name] = V;
//   }
//   // cache the reachable types
//   bidigraph_t tc;
//   boost::transitive_closure(TypeGraph, tc);
//   for (auto V : boost::make_iterator_range(boost::vertices(TypeGraph))) {
//     for (auto OE : boost::make_iterator_range(boost::out_edges(V, tc))) {
//       auto Source = boost::source(OE, tc);
//       auto Target = boost::target(OE, tc);
//       TypeGraph[V].reachableTypes.insert(TypeGraph[Target].name);
//     }
//   }
// }

void LLVMTypeHierarchy::printAsDot(std::ostream &OS) const {
  boost::write_graphviz(OS, TypeGraph,
                        makeTypeHierarchyVertexWriter(TypeGraph));
}

void LLVMTypeHierarchy::printAsJson(std::ostream &OS) const {
  OS << getAsJson();
}

// void LLVMTypeHierarchy::printGraphAsDot(ostream &out) {
//   boost::dynamic_properties dp;
//   dp.property("node_id", get(&LLVMTypeHierarchy::VertexProperties::name,
//   TypeGraph)); boost::write_graphviz_dp(out, TypeGraph, dp);
// }

// LLVMTypeHierarchy::bidigraph_t
// LLVMTypeHierarchy::loadGraphFormDot(istream &in) {
//   LLVMTypeHierarchy::bidigraph_t G(0);
//   boost::dynamic_properties dp;
//   dp.property("node_id", get(&LLVMTypeHierarchy::VertexProperties::name,
//   G)); boost::read_graphviz(in, G, dp); return G;
// }

} // namespace psr
