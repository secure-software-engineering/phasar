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

#include <iostream>
#include <algorithm>

#include <boost/log/sources/record_ostream.hpp>

#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/property_map/dynamic_property_map.hpp>

#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h> // llvm::ConstantArray

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>
#include <phasar/Utils/PAMM.h>

using namespace psr;
using namespace std;

namespace psr {

using json = LLVMTypeHierarchy::json;

LLVMTypeHierarchy::LLVMTypeHierarchy(ProjectIRDB &IRDB) {
  PAMM_FACTORY;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Construct type hierarchy");
  for (auto M : IRDB.getAllModules()) {
    analyzeModule(*M);
    reconstructVTable(*M);
  }
  REG_COUNTER_WITH_VALUE("LTH Vertices", getNumOfVertices());
  REG_COUNTER_WITH_VALUE("LTH Edges", getNumOfEdges());

  bidigraph_t tc;
  boost::transitive_closure(g, tc);

  typename boost::graph_traits<bidigraph_t>::out_edge_iterator ei, ei_end;

  for ( auto vertex : type_vertex_map ) {
    tie(ei, ei_end) = boost::out_edges(vertex.second, tc);
    for (; ei != ei_end; ++ei) {

      auto source = boost::source(*ei, tc);
      auto target = boost::target(*ei, tc);
      g[vertex.second].reachableTypes.insert(g[target].name);
    }
  }

  //NOTE : Interesting statistic as CHA and RTA should only depends on that
  //       and the total number of IR LoC
  //Only for mesure of performance
  // bidigraph_t tc;
  // boost::transitive_closure(g, tc);
  //
  // unsigned int max = 0, total = 0;
  // typename boost::graph_traits<bidigraph_t>::out_edge_iterator ei, ei_end;
  //
  // for ( auto vertex : type_vertex_map ) {
  //   tie(ei, ei_end) = boost::out_edges(vertex.second, tc);
  //   unsigned int dist = distance(ei, ei_end);
  //   max = dist > max ? dist : max;
  //   total += dist;
  // }
  //
  // REG_COUNTER_WITH_VALUE("LTH Max Sub-graph", max);
  // REG_COUNTER_WITH_VALUE("LTH Total Sub-graph", total);
  // REG_COUNTER_WITH_VALUE("LTH Mean Sub-graph", double(total)/double(getNumOfVertices()));
}

void LLVMTypeHierarchy::reconstructVTable(const llvm::Module &M) {
  PAMM_FACTORY;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Reconstruct virtual function table for module: "
                           << M.getModuleIdentifier());
  const static string vtable_for = "vtable for ";
  llvm::Module &m = const_cast<llvm::Module &>(M);
  for (auto &global : m.globals()) {
    string demangled = cxx_demangle(global.getName().str());
    // cxx_demangle returns an empty string if something goes wrong
    if (demangled == "")
      continue;
    // We don't want to find the global "construction vtable for" so
    // we force the start of the global demangled name to start with vtable_for
    //NB: We could also check the mangled name and check that it start with _ZTV
    if (llvm::isa<llvm::Constant>(global) &&
        demangled.find(vtable_for) == 0) {
      llvm::Constant *initializer =
          (global.hasInitializer()) ? global.getInitializer() : nullptr;
      // ignore 'vtable for __cxxabiv1::__si_class_type_info', also the vtable
      // might be marked as external!
      if (!initializer)
        continue;

      // Wrong as clang generate types with a .{n} with n a number for template
      // instance of a class but the vtable is mangled using the whole type
      // string struct_name = demangled.erase(0, vtable_for.size());
      // Better implementation but slow
      if (global.user_empty())
        continue;

      // The first use return a ConstExpr (GetElementPtr) inside a ConstExpr (Bitcast) inside a store
      // We need to access directly the store as the ConstExpr are not linked to a basic bloc and so
      // they can not be printed, we can not access the function in which they are directly, ...
      // We use ++user_begin() at the beginning to avoid finding the VTT, which will currently crash the program
      if (global.user_empty()) {
        throw runtime_error("the vtable has no user");
      }

      auto base = global.user_begin();
      while (base != global.user_end() && (base->user_empty() || base->user_begin()->user_empty() || llvm::isa<llvm::Constant>(*(base->user_begin()->user_begin())))) {
        ++base;
      }

      if (base == global.user_end()) {
        continue;
      }

      // We found a constructor or a destructor
      auto store_vtable_inst = llvm::dyn_cast<llvm::Instruction>(*(base->user_begin()->user_begin()));
      if (store_vtable_inst == nullptr)
        throw runtime_error("store_vtable_inst == nullptr");
      const auto function = store_vtable_inst->getFunction();
      if (function == nullptr)
        throw runtime_error("function found for vtable is a nullptr");

      auto arg_it = function->arg_begin();
      if (arg_it == function->arg_end())
        throw runtime_error("function using vtable has no argument");

      auto this_arg_ty = stripPointer(arg_it->getType());
      auto struct_name = uniformTypeName(this_arg_ty->getStructName().str());

      if (recognized_struct_types.find(struct_name) == recognized_struct_types.end())
        throw runtime_error("found a vtable that doesn't have any node in the class hierarchy");

      // We can prune the hierarchy graph with the knowledge of the vtable
      pruneTypeHierarchyWithVtable(function);
      // check if the vtable is already initialized, then we can skip
      if (vtable_map.find(struct_name) != vtable_map.end())
        continue;

      for (unsigned i = 0; i < initializer->getNumOperands(); ++i) {
        if (llvm::ConstantArray *constant_array =
                llvm::dyn_cast<llvm::ConstantArray>(
                    initializer->getAggregateElement(i))) {
          for (unsigned j = 0; j < constant_array->getNumOperands(); ++j) {
            if (llvm::ConstantExpr *constant_expr =
                    llvm::dyn_cast<llvm::ConstantExpr>(
                        constant_array->getAggregateElement(j))) {
              if (constant_expr->isCast()) {
                if (llvm::Constant *cast = llvm::ConstantExpr::getBitCast(
                        constant_expr, constant_expr->getType())) {
                  if (llvm::Function *vfunc =
                          llvm::dyn_cast<llvm::Function>(cast->getOperand(0))) {
                    vtable_map[struct_name].addEntry(vfunc->getName().str());
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

void LLVMTypeHierarchy::analyzeModule(const llvm::Module &M) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Analyse types in module: "
                           << M.getModuleIdentifier());
  // store analyzed module
  contained_modules.insert(&M);
  auto StructTypes = M.getIdentifiedStructTypes();
  for (auto StructType : StructTypes) {
    auto struct_type_name = StructType->getName().str();

    // Avoid to have the struct.Myclass.base in the database, as it is not used
    // by the code anywhere else than in type declaration for alignement reasons


    if (struct_type_name.compare(struct_type_name.size() - sizeof(".base") + 1,
                                 sizeof(".base") - 1, ".base") != 0) {
      struct_type_name = psr::uniformTypeName(struct_type_name);

      // only add a new vertex to the graph if the type is currently unknown!
      if (recognized_struct_types.find(struct_type_name) ==
          recognized_struct_types.end()) {
        type_vertex_map[struct_type_name] = boost::add_vertex(g);
        g[type_vertex_map[struct_type_name]].llvmtype = StructType;
        g[type_vertex_map[struct_type_name]].name = StructType->getName().str();
        g[type_vertex_map[struct_type_name]].reachableTypes.insert(g[type_vertex_map[struct_type_name]].name);

      }
    }
  }
  // construct the edges between a type and its subtypes
  for (auto StructType : StructTypes) {
    auto struct_type_name = psr::uniformTypeName(StructType->getName().str());

    for (auto Subtype : StructType->subtypes()) {
      if (Subtype->isStructTy()) {
        llvm::StructType *StructSubType =
            llvm::dyn_cast<llvm::StructType>(Subtype);
        auto struct_sub_type_name = psr::uniformTypeName(StructSubType->getName().str());

        boost::add_edge(type_vertex_map[struct_sub_type_name],
                        type_vertex_map[struct_type_name], g);
      }
    }
  }
  for_each(StructTypes.begin(), StructTypes.end(),
           [this](const llvm::StructType *ST) {
             auto struct_type_name = psr::uniformTypeName(ST->getName().str());

             recognized_struct_types.insert(struct_type_name);
           });
}

void LLVMTypeHierarchy::pruneTypeHierarchyWithVtable(const llvm::Function* constructor) {
  if (constructor == nullptr)
    throw runtime_error("constructor found for vtable is a nullptr");

  auto arg_it = constructor->arg_begin();
  if (arg_it == constructor->arg_end())
    throw runtime_error("constructor using vtable has no argument");

  auto this_arg_ty = stripPointer(arg_it->getType());
  auto this_ty_name = uniformTypeName(this_arg_ty->getStructName().str());

  if (recognized_struct_types.find(this_ty_name) == recognized_struct_types.end())
    throw runtime_error("found a vtable that doesn't have any node in the class hierarchy");

  unsigned i = 0, vtable_pos = 0;
  set<string> pre_vtable, post_vtable;
  for ( auto I = llvm::inst_begin(constructor), E = llvm::inst_end(constructor); I != E;
       ++I, ++i ) {
    const auto& Inst = *I;

    if ( auto store = llvm::dyn_cast<llvm::StoreInst>(&Inst) ) {
      // We got a store instruction, now we are checking if it is a vtable storage
      if ( auto bitcast_expr = llvm::dyn_cast<llvm::ConstantExpr>(store->getValueOperand()) ) {
        if ( bitcast_expr->isCast() ) {
          if ( auto const_gep = llvm::dyn_cast<llvm::ConstantExpr>(bitcast_expr->getOperand(0)) ) {
            if ( auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(const_gep->getAsInstruction()) ) {
              if ( auto vtable = llvm::dyn_cast<llvm::Constant>(gep->getPointerOperand()) ) {
                // We can here assume that we found a vtable
                vtable_pos = i;
              }
            }
          }
        }
      }
    }

    if ( auto call_inst = llvm::dyn_cast<llvm::CallInst>(&Inst) ) {
      if ( auto called = call_inst->getCalledFunction() ) {
        if ( isConstructor(called->getName().str()) ) {
          if ( auto this_type = called->getFunctionType()->getParamType(0) ) {
            if ( auto struct_ty = llvm::dyn_cast<llvm::StructType>(stripPointer(this_type))) {
              auto struct_name = uniformTypeName(struct_ty->getName().str());
              if ( vtable_pos == 0 )
                pre_vtable.insert(struct_name);
              else
                post_vtable.insert(struct_name);
            }
          }
        }
      }
    }
  }

  for ( auto post_cons : post_vtable ) {
    if ( pre_vtable.find(post_cons) != pre_vtable.end() ) {
      post_vtable.erase(post_cons);
    }
  }

  auto u = type_vertex_map[this_ty_name];
  for ( auto post_ty_name : post_vtable ) {
    auto v = type_vertex_map[post_ty_name];
    if ( boost::edge(v, u, g).second ) {
      boost::remove_edge(v, u, g);
    }
  }
}

set<string> LLVMTypeHierarchy::getTransitivelyReachableTypes(string TypeName) {
  TypeName = psr::uniformTypeName(TypeName);

  return g[type_vertex_map[TypeName]].reachableTypes;
}

string LLVMTypeHierarchy::getVTableEntry(string TypeName, unsigned idx) const {
  TypeName = psr::uniformTypeName(TypeName);



  const auto iter = vtable_map.find(TypeName);
  if (iter != vtable_map.end()) {
    return iter->second.getFunctionByIdx(idx);
  }

  return "";
}

VTable LLVMTypeHierarchy::getVTable(string TypeName) {
  TypeName = psr::uniformTypeName(TypeName);

  return vtable_map[TypeName];
}

bool LLVMTypeHierarchy::hasSuperType(string TypeName, string SuperTypeName) {
  return hasSubType(SuperTypeName, TypeName);
}

bool LLVMTypeHierarchy::hasSubType(string TypeName, string SubTypeName) {
  TypeName = psr::uniformTypeName(TypeName);
  SubTypeName = psr::uniformTypeName(SubTypeName);

  auto reachable_types = getTransitivelyReachableTypes(TypeName);
  return reachable_types.find(SubTypeName) != reachable_types.end();
}

bool LLVMTypeHierarchy::containsVTable(string TypeName) const {
  TypeName = psr::uniformTypeName(TypeName);

  auto iter = vtable_map.find(TypeName);
  return iter != vtable_map.end();
}

bool LLVMTypeHierarchy::containsType(string TypeName) const {
  TypeName = psr::uniformTypeName(TypeName);

  return recognized_struct_types.count(TypeName);
}

string LLVMTypeHierarchy::getPlainTypename(string TypeName) {
  // types are named something like: 'struct.MyType' or 'struct.MyType.base'
  return psr::uniformTypeName(TypeName);
}

void LLVMTypeHierarchy::print() {
  cout << "LLVMSructTypeHierarchy graph:\n";
  boost::print_graph(g,
                     boost::get(&LLVMTypeHierarchy::VertexProperties::name, g));
  cout << "\nVTables:\n";
  if (vtable_map.empty()) {
    cout << "EMPTY\n";
  } else {
    for (const auto entry : vtable_map) {
      cout << entry.first << " contains\n" << entry.second << endl;
    }
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
  vertex_iterator vi_v, vi_v_end;
  out_edge_iterator ei, ei_end;
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
