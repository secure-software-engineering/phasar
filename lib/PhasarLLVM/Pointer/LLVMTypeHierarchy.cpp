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

#include <boost/log/sources/record_ostream.hpp>

#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <boost/property_map/dynamic_property_map.hpp>

#include <llvm/IR/Constants.h> // llvm::ConstantArray
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/GraphExtensions.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>
#include <phasar/Utils/PAMMMacros.h>

using namespace psr;
using namespace std;

namespace psr {

using json = LLVMTypeHierarchy::json;

LLVMTypeHierarchy::VertexProperties::VertexProperties(llvm::StructType *Type,
                                                      std::string TypeName)
    : llvmtype(Type), name(TypeName) {
  reachableTypes.insert(TypeName);
}

LLVMTypeHierarchy::LLVMTypeHierarchy(ProjectIRDB &IRDB) {
  PAMM_GET_INSTANCE;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Construct type hierarchy");
  for (auto M : IRDB.getAllModules()) {
    buildLLVMTypeHierarchy(*M);
  }
  REG_COUNTER("CH Vertices", getNumOfVertices(), PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("CH Edges", getNumOfEdges(), PAMM_SEVERITY_LEVEL::Full);
}

LLVMTypeHierarchy::LLVMTypeHierarchy(const llvm::Module &M) {
  PAMM_GET_INSTANCE;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Construct type hierarchy");
  buildLLVMTypeHierarchy(M);
  REG_COUNTER("CH Vertices", getNumOfVertices(), PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("CH Edges", getNumOfEdges(), PAMM_SEVERITY_LEVEL::Full);
}

void LLVMTypeHierarchy::buildLLVMTypeHierarchy(const llvm::Module &M) {
  // build the hierarchy for the module
  constructHierarchy(M);
  // reconstruct all available vtables
  reconstructVTables(M);
  // cache the reachable types
  bidigraph_t tc;
  boost::transitive_closure(g, tc);
  for (auto V : boost::make_iterator_range(boost::vertices(g))) {
    for (auto OE : boost::make_iterator_range(boost::out_edges(V, tc))) {
      auto Source = boost::source(OE, tc);
      auto Target = boost::target(OE, tc);
      g[V].reachableTypes.insert(g[Target].name);
    }
  }
}

void LLVMTypeHierarchy::reconstructVTables(const llvm::Module &M) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Reconstruct virtual function table for module: "
                << M.getModuleIdentifier());
  const static string VTableFor = "vtable for";
  llvm::Module &m = const_cast<llvm::Module &>(M);
  for (auto &Global : m.globals()) {
    string DemangledGlobalName = cxx_demangle(Global.getName().str());
    // cxx_demangle returns an empty string if something goes wrong
    if (DemangledGlobalName == "")
      continue;
    // We don't want to find the global "construction vtable for" so
    // we force the start of the global demangled name to start with vtable_for
    // NB: We could also check the mangled name and check that it start with
    // _ZTV
    if (llvm::isa<llvm::Constant>(Global) &&
        DemangledGlobalName.find(VTableFor) == 0) {
      llvm::Constant *GlobalInitializer =
          (Global.hasInitializer()) ? Global.getInitializer() : nullptr;
      // ignore 'vtable for __cxxabiv1::__si_class_type_info', also the vtable
      // might be marked as external!
      if (!GlobalInitializer)
        continue;
      // Wrong as clang generate types with a .{n} with n a number for template
      // instance of a class but the vtable is mangled using the whole type
      // string struct_name = demangled.erase(0, vtable_for.size());
      // Better implementation but slow
      if (Global.user_empty())
        continue;
      // The first use return a ConstExpr (GetElementPtr) inside a ConstExpr
      // (Bitcast) inside a store We need to access directly the store as the
      // ConstExpr are not linked to a basic bloc and so they can not be
      // printed, we can not access the function in which they are directly, ...
      // We use ++user_begin() at the beginning to avoid finding the VTT, which
      // will currently crash the program
      if (Global.user_empty()) {
        throw runtime_error("The vtable " + DemangledGlobalName +
                            " has no user!");
      }

      auto Base = Global.user_begin();
      while (Base != Global.user_end() &&
             (Base->user_empty() || Base->user_begin()->user_empty() ||
              llvm::isa<llvm::Constant>(*(Base->user_begin()->user_begin())))) {
        ++Base;
      }

      if (Base == Global.user_end()) {
        continue;
      }

      // We found a constructor or a destructor
      auto StoreVtableInst = llvm::dyn_cast<llvm::Instruction>(
          *(Base->user_begin()->user_begin()));
      if (StoreVtableInst == nullptr) {
        throw runtime_error("store_vtable_inst == nullptr");
      }
      const auto Function = StoreVtableInst->getFunction();
      if (Function == nullptr) {
        throw runtime_error("function found for vtable is a nullptr");
      }

      if (!Function->arg_size()) {
        throw runtime_error("function using vtable has no argument");
      }
      auto ArgIt = Function->arg_begin();
      auto ArgTy = stripPointer(ArgIt->getType());
      auto StructName = ArgTy->getStructName().str();
      StructName = debasify(StructName);
      if (!containsType(StructName)) {
        throw runtime_error(
            "found a vtable that doesn't have any node in the class hierarchy");
      }
      // We can prune the hierarchy graph with the knowledge of the vtable
      pruneTypeHierarchyWithVtable(Function);
      // check if the vtable is already initialized, then we can skip
      if (type_vtbl_map.count(StructName))
        continue;

      for (unsigned i = 0; i < GlobalInitializer->getNumOperands(); ++i) {
        if (llvm::ConstantArray *ConstArray =
                llvm::dyn_cast<llvm::ConstantArray>(
                    GlobalInitializer->getAggregateElement(i))) {
          for (unsigned j = 0; j < ConstArray->getNumOperands(); ++j) {
            if (llvm::ConstantExpr *ConstExpr =
                    llvm::dyn_cast<llvm::ConstantExpr>(
                        ConstArray->getAggregateElement(j))) {
              if (ConstExpr->isCast()) {
                if (llvm::Constant *Cast = llvm::ConstantExpr::getBitCast(
                        ConstExpr, ConstExpr->getType())) {
                  if (llvm::Function *VirtualFunction =
                          llvm::dyn_cast<llvm::Function>(Cast->getOperand(0))) {
                    addVTableEntry(StructName,
                                   VirtualFunction->getName().str());
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

void LLVMTypeHierarchy::constructHierarchy(const llvm::Module &M) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Analyse types in module: " << M.getModuleIdentifier());
  // store analyzed module
  contained_modules.insert(&M);
  // iterate struct types and draw the edges
  auto StructTypes = M.getIdentifiedStructTypes();
  for (auto StructType : StructTypes) {
    auto TypeName = StructType->getName().str();
    // Avoid to have the struct.MyType.base in the database, as it is not used
    // by the code anywhere else than in type declaration for alignement reasons
    string DebTypeName = psr::debasify(TypeName);
    // handle special cases where .base actually makes sense
    if (!M.getTypeByName(DebTypeName)) {
      DebTypeName = TypeName;
    }
    // only add a new vertex to the graph if the type is currently unknown!
    if (!type_vertex_map.count(DebTypeName)) {
      auto Vertex = boost::add_vertex(g);
      type_vertex_map[DebTypeName] = Vertex;
      auto StructTypePtr = M.getTypeByName(DebTypeName);
      assert(StructTypePtr && "Module does not contain requested type!");
      g[Vertex] = VertexProperties(StructTypePtr, DebTypeName);
    }
  }
  // construct the edges between a type and its subtypes
  for (auto StructType : StructTypes) {
    auto TypeName = StructType->getName().str();
    TypeName = psr::debasify(TypeName);
    for (auto SubType : StructType->subtypes()) {
      // check if the subtype is a struct
      if (llvm::StructType *StructSubType =
              llvm::dyn_cast<llvm::StructType>(SubType)) {
        auto SubTypeName = StructSubType->getName().str();
        SubTypeName = debasify(SubTypeName);
        boost::add_edge(type_vertex_map[SubTypeName], type_vertex_map[TypeName],
                        g);
      }
    }
  }
}

void LLVMTypeHierarchy::pruneTypeHierarchyWithVtable(
    const llvm::Function *Constructor) {
  if (!Constructor) {
    throw runtime_error("constructor found for vtable is a nullptr");
  }
  auto ArgIt = Constructor->arg_begin();
  if (ArgIt == Constructor->arg_end()) {
    throw runtime_error("constructor using vtable has no argument");
  }
  auto ArgTy = stripPointer(ArgIt->getType());
  auto TypeName = ArgTy->getStructName().str();
  TypeName = debasify(TypeName);
  if (!containsType(TypeName)) {
    throw runtime_error(
        "found a vtable that doesn't have any node in the class hierarchy");
  }
  unsigned i = 0, vtable_pos = 0;
  set<string> pre_vtable, post_vtable;
  for (auto I = llvm::inst_begin(Constructor), E = llvm::inst_end(Constructor);
       I != E; ++I, ++i) {
    const auto &Inst = *I;

    if (auto store = llvm::dyn_cast<llvm::StoreInst>(&Inst)) {
      // We got a store instruction, now we are checking if it is a vtable
      // storage
      if (auto bitcast_expr =
              llvm::dyn_cast<llvm::ConstantExpr>(store->getValueOperand())) {
        if (bitcast_expr->isCast()) {
          if (auto const_gep = llvm::dyn_cast<llvm::ConstantExpr>(
                  bitcast_expr->getOperand(0))) {
            auto gep_as_inst = const_gep->getAsInstruction();
            if (auto gep =
                    llvm::dyn_cast<llvm::GetElementPtrInst>(gep_as_inst)) {
              if (auto vtable = llvm::dyn_cast<llvm::Constant>(
                      gep->getPointerOperand())) {
                // We can here assume that we found a vtable
                vtable_pos = i;
              }
            }
            gep_as_inst->deleteValue();
          }
        }
      }
    }

    if (auto call_inst = llvm::dyn_cast<llvm::CallInst>(&Inst)) {
      if (auto called = call_inst->getCalledFunction()) {
        if (isConstructor(called->getName().str())) {
          if (auto this_type = called->getFunctionType()->getParamType(0)) {
            if (auto struct_ty =
                    llvm::dyn_cast<llvm::StructType>(stripPointer(this_type))) {
              auto struct_name = debasify(struct_ty->getName().str());
              if (vtable_pos == 0)
                pre_vtable.insert(struct_name);
              else
                post_vtable.insert(struct_name);
            }
          }
        }
      }
    }
  }

  for (auto post_cons : post_vtable) {
    if (pre_vtable.find(post_cons) != pre_vtable.end()) {
      post_vtable.erase(post_cons);
    }
  }

  auto u = type_vertex_map[TypeName];
  for (auto post_ty_name : post_vtable) {
    auto v = type_vertex_map[post_ty_name];
    if (boost::edge(v, u, g).second) {
      boost::remove_edge(v, u, g);
    }
  }
}

set<string> LLVMTypeHierarchy::getTransitivelyReachableTypes(string TypeName) {
  TypeName = debasify(TypeName);
  return g[type_vertex_map[TypeName]].reachableTypes;
}

string LLVMTypeHierarchy::getVTableEntry(string TypeName, unsigned idx) const {
  return getVTable(TypeName).getFunctionByIdx(idx);
}

VTable LLVMTypeHierarchy::getVTable(string TypeName) const {
  return type_vtbl_map.at(TypeName);
}

bool LLVMTypeHierarchy::hasSuperType(string TypeName, string SuperTypeName) {
  return hasSubType(SuperTypeName, TypeName);
}

size_t LLVMTypeHierarchy::getNumTypes() const { return type_vertex_map.size(); }

size_t LLVMTypeHierarchy::getNumVTableEntries(std::string TypeName) const {
  return getVTable(TypeName).size();
}

bool LLVMTypeHierarchy::hasSubType(string TypeName, string SubTypeName) {
  TypeName = debasify(TypeName);
  SubTypeName = debasify(SubTypeName);
  auto ReachableTypes = getTransitivelyReachableTypes(TypeName);
  return ReachableTypes.count(SubTypeName);
}

bool LLVMTypeHierarchy::containsVTable(string TypeName) const {
  return type_vtbl_map.count(TypeName);
}

bool LLVMTypeHierarchy::containsType(string TypeName) const {
  return type_vertex_map.count(debasify(TypeName));
}

void LLVMTypeHierarchy::addVTableEntry(std::string TypeName,
                                       std::string FunctionName) {
  type_vtbl_map[TypeName].addEntry(FunctionName);
}

const llvm::StructType *LLVMTypeHierarchy::getType(std::string TypeName) const {
  return g[type_vertex_map.at(TypeName)].llvmtype;
}

void LLVMTypeHierarchy::mergeWith(LLVMTypeHierarchy &Other) {
  cout << "LLVMTypeHierarchy::mergeWith()" << endl;
  boost::copy_graph(Other.g, g); // G += H;
  // build the contractions
  vector<pair<vertex_t, vertex_t>> contractions;
  map<string, vertex_t> observed;
  for (auto V : boost::make_iterator_range(boost::vertices(g))) {
    if (observed.find(g[V].name) != observed.end()) {
      // check which one has the valid pointer and the knowledge of the vtables
      if (g[V].llvmtype) {
        contractions.push_back(make_pair(observed[g[V].name], V));
      } else {
        contractions.push_back(make_pair(V, observed[g[V].name]));
      }
    } else {
      observed[g[V].name] = V;
    }
  }
  cout << "contractions.size(): " << contractions.size() << '\n';
  for (auto contraction : contractions) {
    contract_vertices<bidigraph_t, vertex_t, EdgeProperties>(
        contraction.first, contraction.second, g);
  }
  // merge the vtables
  type_vtbl_map.insert(Other.type_vtbl_map.begin(), Other.type_vtbl_map.end());
  // merge the modules analyzed
  contained_modules.insert(Other.contained_modules.begin(),
                           Other.contained_modules.end());
  // reset the vertex mapping
  type_vertex_map.clear();
  for (auto V : boost::make_iterator_range(boost::vertices(g))) {
    type_vertex_map[g[V].name] = V;
  }
  // cache the reachable types
  bidigraph_t tc;
  boost::transitive_closure(g, tc);
  for (auto V : boost::make_iterator_range(boost::vertices(g))) {
    for (auto OE : boost::make_iterator_range(boost::out_edges(V, tc))) {
      auto Source = boost::source(OE, tc);
      auto Target = boost::target(OE, tc);
      g[V].reachableTypes.insert(g[Target].name);
    }
  }
}

void LLVMTypeHierarchy::print() {
  cout << "LLVMSructTypeHierarchy graph:\n";
  boost::print_graph(g,
                     boost::get(&LLVMTypeHierarchy::VertexProperties::name, g));
  cout << "\nVTables:\n";
  for (auto VTable : type_vtbl_map) {
    cout << VTable.first << '\n';
    cout << VTable.second << '\n';
  }
}

void LLVMTypeHierarchy::printAsDot(const string &path) {
  ofstream ofs(path);
  boost::write_graphviz(
      ofs, g, boost::make_label_writer(boost::get(&VertexProperties::name, g)));
}

void LLVMTypeHierarchy::printGraphAsDot(ostream &out) {
  boost::dynamic_properties dp;
  dp.property("node_id", get(&LLVMTypeHierarchy::VertexProperties::name, g));
  boost::write_graphviz_dp(out, g, dp);
}

LLVMTypeHierarchy::bidigraph_t
LLVMTypeHierarchy::loadGraphFormDot(istream &in) {
  LLVMTypeHierarchy::bidigraph_t G(0);
  boost::dynamic_properties dp;
  dp.property("node_id", get(&LLVMTypeHierarchy::VertexProperties::name, G));
  boost::read_graphviz(in, G, dp);
  return G;
}

void LLVMTypeHierarchy::printTransitiveClosure() {
  bidigraph_t tc;
  boost::transitive_closure(g, tc);
  boost::print_graph(tc,
                     boost::get(&LLVMTypeHierarchy::VertexProperties::name, g));
}

json LLVMTypeHierarchy::getAsJson() {
  json J;
  vertex_iterator_t vi_v, vi_v_end;
  out_edge_iterator_t ei, ei_end;
  // iterate all graph vertices
  for (boost::tie(vi_v, vi_v_end) = boost::vertices(g); vi_v != vi_v_end;
       ++vi_v) {
    J[JsonTypeHierarchyID][g[*vi_v].name];
    // iterate all out edges of vertex vi_v
    for (boost::tie(ei, ei_end) = boost::out_edges(*vi_v, g); ei != ei_end;
         ++ei) {
      J[JsonTypeHierarchyID][g[*vi_v].name] += g[boost::target(*ei, g)].name;
    }
  }
  return J;
}

unsigned LLVMTypeHierarchy::getNumOfVertices() {
  return boost::num_vertices(g);
}

unsigned LLVMTypeHierarchy::getNumOfEdges() { return boost::num_edges(g); }
} // namespace psr
