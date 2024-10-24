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

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/Format.h"

#include "boost/graph/graphviz.hpp"
#include "boost/graph/transitive_closure.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <ostream>

using namespace std;

namespace psr {

// provide a VertexPropertyWrite to tell boost how to write a vertex
class TypeHierarchyVertexWriter {
public:
  TypeHierarchyVertexWriter(const LLVMTypeHierarchy::bidigraph_t &TyGraph)
      : TyGraph(TyGraph) {}
  template <class VertexOrEdge>
  void operator()(std::ostream &Out, const VertexOrEdge &V) const {
    Out << "[label=\"" << TyGraph[V].getTypeName() << "\"]";
  }

private:
  const LLVMTypeHierarchy::bidigraph_t &TyGraph;
};

LLVMTypeHierarchy::VertexProperties::VertexProperties(
    const llvm::StructType *Type)
    : Type(Type), ReachableTypes({Type}) {}

std::string LLVMTypeHierarchy::VertexProperties::getTypeName() const {
  return Type->getStructName().str();
}

LLVMTypeHierarchy::LLVMTypeHierarchy(const LLVMProjectIRDB &IRDB) {
  PHASAR_LOG_LEVEL(INFO, "Construct type hierarchy");
  buildLLVMTypeHierarchy(*IRDB.getModule());
}

LLVMTypeHierarchy::LLVMTypeHierarchy(
    const LLVMProjectIRDB &IRDB, const LLVMTypeHierarchyData &SerializedData) {

  llvm::StringMap<llvm::StructType *> NameToStructType;
  const auto &IRDBModule = IRDB.getModule();

  VisitedModules.insert(IRDBModule);
  auto StructTypes = IRDBModule->getIdentifiedStructTypes();

  // find all struct types by name
  for (const auto &SerElement : SerializedData.TypeGraph) {
    bool MatchFound = false;

    for (const auto &StructTypeElement : StructTypes) {
      if (SerElement.getKey() == StructTypeElement->getName()) {
        NameToStructType.try_emplace(SerElement.getKey(), StructTypeElement);
        MatchFound = true;
        break;
      }
    }

    if (!MatchFound) {
      PHASAR_LOG_LEVEL(WARNING,
                       "No matching StructType found for Type with name: "
                           << SerElement.getKey());
    }
  }

  // add all vertices
  for (const auto &Curr : NameToStructType) {
    const auto &StructType = Curr.getValue();
    auto Vertex = boost::add_vertex(TypeGraph);
    TypeVertexMap[StructType] = Vertex;
    TypeGraph[Vertex] = VertexProperties(StructType);
    TypeVFTMap[StructType] = getVirtualFunctions(*IRDBModule, *StructType);
  }

  // add all edges
  for (const auto &SerElement : SerializedData.TypeGraph) {
    const auto *SrcType = NameToStructType[SerElement.getKey()];
    if (!SrcType) {
      continue;
    }
    auto Vtx = TypeVertexMap.at(SrcType);
    for (const auto &CurrEdge : SerElement.getValue()) {
      const auto *DestType = NameToStructType[CurrEdge];
      if (!SrcType) {
        continue;
      }
      auto DestVtx = TypeVertexMap.at(DestType);

      TypeGraph[Vtx].ReachableTypes.insert(DestType);
      boost::add_edge(Vtx, DestVtx, TypeGraph);
    }
  }
}

LLVMTypeHierarchy::LLVMTypeHierarchy(const llvm::Module &M) {
  PHASAR_LOG_LEVEL_CAT(INFO, "LLVMTypeHierarchy", "Construct type hierarchy");
  buildLLVMTypeHierarchy(M);
  PHASAR_LOG_LEVEL_CAT(INFO, "LLVMTypeHierarchy", "Finished type hierarchy");
}

std::string
LLVMTypeHierarchy::removeStructOrClassPrefix(const llvm::StructType &T) {
  return removeStructOrClassPrefix(T.getName().str());
}

std::string
LLVMTypeHierarchy::removeStructOrClassPrefix(llvm::StringRef TypeName) {
  if (TypeName.startswith(StructPrefix)) {
    TypeName = TypeName.drop_front(StructPrefix.size());
  } else if (TypeName.startswith(ClassPrefix)) {
    TypeName = TypeName.drop_front(ClassPrefix.size());
  }
  if (TypeName.endswith(".base")) {
    TypeName = TypeName.drop_back(llvm::StringRef(".base").size());
  }
  return TypeName.str();
}

std::string LLVMTypeHierarchy::removeTypeInfoPrefix(llvm::StringRef VarName) {
  if (VarName.startswith(TypeInfoPrefixDemang)) {
    return VarName.drop_front(TypeInfoPrefixDemang.size()).str();
  }
  if (VarName.startswith(TypeInfoPrefix)) {
    return VarName.drop_front(TypeInfoPrefix.size()).str();
  }
  return VarName.str();
}

std::string LLVMTypeHierarchy::removeVTablePrefix(llvm::StringRef VarName) {
  if (VarName.startswith(VTablePrefixDemang)) {
    return VarName.drop_front(VTablePrefixDemang.size()).str();
  }
  if (VarName.startswith(VTablePrefix)) {
    return VarName.drop_front(VTablePrefix.size()).str();
  }
  return VarName.str();
}

bool LLVMTypeHierarchy::isTypeInfo(llvm::StringRef VarName) {
  if (VarName.startswith("_ZTI")) {
    return true;
  }
  // In LLVM 16 demangle() takes a StringRef
  auto Demang = llvm::demangle(VarName.str());
  return llvm::StringRef(Demang).startswith(TypeInfoPrefixDemang);
}

bool LLVMTypeHierarchy::isVTable(llvm::StringRef VarName) {
  if (VarName.startswith("_ZTV")) {
    return true;
  }
  // In LLVM 16 demangle() takes a StringRef
  auto Demang = llvm::demangle(VarName.str());
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
                               const llvm::StructType &Type) const {
  // find corresponding type info variable
  std::vector<const llvm::StructType *> SubTypes;
  std::string ClearName = removeStructOrClassPrefix(Type);

  if (auto It = ClearNameTIMap.find(ClearName); It != ClearNameTIMap.end()) {
    const auto *TI = It->second;
    if (!TI->hasInitializer()) {
      PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMTypeHierarchy",
                           ClearName << " does not have initializer");
      return SubTypes;
    }
    if (const auto *I =
            llvm::dyn_cast<llvm::ConstantStruct>(TI->getInitializer())) {
      for (const auto &Op : I->operands()) {
        const auto *CE = Op->stripPointerCastsAndAliases();

        if (CE->hasName()) {
          auto Name = CE->getName();
          if (Name.find(TypeInfoPrefix) != llvm::StringRef::npos) {
            auto ClearName = removeTypeInfoPrefix(llvm::demangle(Name.str()));
            if (auto TyIt = ClearNameTypeMap.find(ClearName);
                TyIt != ClearNameTypeMap.end()) {
              SubTypes.push_back(TyIt->second);
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
  auto ClearName = removeStructOrClassPrefix(Type.getName());
  std::vector<const llvm::Function *> VFS;
  if (const auto *TV = ClearNameTVMap[ClearName]) {
    if (const auto *TI = llvm::dyn_cast<llvm::GlobalVariable>(TV)) {
      if (!TI->hasInitializer()) {
        PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMTypeHierarchy",
                             ClearName << " does not have initializer");
        return VFS;
      }
      if (const auto *I =
              llvm::dyn_cast<llvm::ConstantStruct>(TI->getInitializer())) {
        VFS = LLVMVFTable::getVFVectorFromIRVTable(*I);
      }
    }
  }
  return VFS;
}

void LLVMTypeHierarchy::constructHierarchy(const llvm::Module &M) {
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMTypeHierarchy",
                       "Analyze types in module: " << M.getModuleIdentifier());
  // store analyzed module
  VisitedModules.insert(&M);
  auto StructTypes = M.getIdentifiedStructTypes();
  // build helper maps
  for (auto *StructType : StructTypes) {
    ClearNameTypeMap[removeStructOrClassPrefix(*StructType)] = StructType;
  }
  for (const auto &Global : M.globals()) {
    if (Global.hasName()) {
      if (isTypeInfo(Global.getName())) {
        auto Demang = llvm::demangle(Global.getName().str());
        auto ClearName = removeTypeInfoPrefix(Demang);
        ClearNameTIMap[ClearName] = &Global;
      }
      if (isVTable(Global.getName())) {
        auto Demang = llvm::demangle(Global.getName().str());
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
LLVMTypeHierarchy::getSubTypes(const llvm::StructType *Type) const {
  if (TypeVertexMap.count(Type)) {
    if (auto It = TypeVertexMap.find(Type); It != TypeVertexMap.end()) {
      return TypeGraph[It->second].ReachableTypes;
    }
  }
  return {};
}

template <typename GraphT>
static const llvm::StructType *getTypeImpl(const GraphT &TypeGraph,
                                           llvm::StringRef TypeName) {
  for (auto V : boost::make_iterator_range(boost::vertices(TypeGraph))) {
    if (TypeGraph[V].Type->getName() == TypeName) {
      return TypeGraph[V].Type;
    }
  }
  return nullptr;
}

const llvm::StructType *
LLVMTypeHierarchy::getType(llvm::StringRef TypeName) const {
  if (const auto *Ty = getTypeImpl(TypeGraph, TypeName)) {
    return Ty;
  }

  return getTypeImpl(TypeGraph, (TypeName + ".base").str());
}

std::vector<const llvm::StructType *> LLVMTypeHierarchy::getAllTypes() const {
  std::vector<const llvm::StructType *> Types;
  Types.reserve(boost::num_vertices(TypeGraph));
  for (auto V : boost::make_iterator_range(boost::vertices(TypeGraph))) {
    Types.push_back(TypeGraph[V].Type);
  }
  return Types;
}

llvm::StringRef
LLVMTypeHierarchy::getTypeName(const llvm::StructType *Type) const {
  return Type->getStructName();
}

void LLVMTypeHierarchy::print(llvm::raw_ostream &OS) const {
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
    OS << "Virtual function table for: " << Ty->getName() << '\n';
    for (const auto *F : VFT) {
      OS << "\t-" << F->getName() << '\n';
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
    J[PhasarConfig::JsonTypeHierarchyID().str()][TypeGraph[*VIv].getTypeName()];
    // iterate all out edges of vertex vi_v
    for (boost::tie(EI, EIEnd) = boost::out_edges(*VIv, TypeGraph); EI != EIEnd;
         ++EI) {
      J[PhasarConfig::JsonTypeHierarchyID().str()]
       [TypeGraph[*VIv].getTypeName()] +=
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

void LLVMTypeHierarchy::printAsDot(llvm::raw_ostream &OS) const {
  std::stringstream S;
  boost::write_graphviz(S, TypeGraph, TypeHierarchyVertexWriter(TypeGraph));
  OS << S.str();
}

void LLVMTypeHierarchy::printAsJson(llvm::raw_ostream &OS) const {
  LLVMTypeHierarchyData Data;
  Data.PhasarConfigJsonTypeHierarchyID =
      PhasarConfig::JsonTypeHierarchyID().str();

  // iterate all graph vertices
  for (auto Vtx : boost::make_iterator_range(boost::vertices(TypeGraph))) {
    //  iterate all out edges of vertex vi_v
    auto &SerTypes = Data.TypeGraph[TypeGraph[Vtx].getTypeName()];
    for (const auto &CurrReachable : TypeGraph[Vtx].ReachableTypes) {
      SerTypes.push_back(CurrReachable->getName().str());
    }
  }

  Data.printAsJson(OS);
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
