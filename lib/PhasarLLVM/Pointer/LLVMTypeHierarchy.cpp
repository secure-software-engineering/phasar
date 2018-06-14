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

#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
using namespace std;
using namespace psr;
namespace psr {

LLVMTypeHierarchy::LLVMTypeHierarchy(ProjectIRDB &IRDB) {
  PAMM_FACTORY;
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, INFO) << "Construct type hierarchy";
  for (auto M : IRDB.getAllModules()) {
    analyzeModule(*M);
    reconstructVTable(*M);
  }
  REG_COUNTER_WITH_VALUE("LTH Vertices", getNumOfVertices());
  REG_COUNTER_WITH_VALUE("LTH Edges", getNumOfEdges());
}

void LLVMTypeHierarchy::reconstructVTable(const llvm::Module &M) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "Reconstruct virtual function table for module: "
                           << M.getModuleIdentifier();
  const static string vtable_for = "vtable for ";
  llvm::Module &m = const_cast<llvm::Module &>(M);
  for (auto &global : m.globals()) {
    string demangled = cxx_demangle(global.getName().str());
    // cxx_demangle returns an empty string if something goes wrong
    if (demangled == "")
      continue;
    if (llvm::isa<llvm::Constant>(global) &&
        demangled.find(vtable_for) != demangled.npos) {
      string struct_name = demangled.erase(0, vtable_for.size());
      llvm::Constant *initializer =
          (global.hasInitializer()) ? global.getInitializer() : nullptr;
      // ignore 'vtable for __cxxabiv1::__si_class_type_info', also the vtable
      // might be marked as external!
      if (!initializer)
        continue;
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
  BOOST_LOG_SEV(lg, DEBUG) << "Analyse types in module: "
                           << M.getModuleIdentifier();
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
        auto struct_sub_type_name =
            psr::uniformTypeName(StructSubType->getName().str());

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

set<string> LLVMTypeHierarchy::getTransitivelyReachableTypes(string TypeName) {
  TypeName = psr::uniformTypeName(TypeName);

  set<string> reachable_nodes;
  bidigraph_t tc;
  boost::transitive_closure(g, tc);

  // get all out edges of queried type
  typename boost::graph_traits<bidigraph_t>::out_edge_iterator ei, ei_end;

  reachable_nodes.insert(g[type_vertex_map[TypeName]].name);
  for (tie(ei, ei_end) = boost::out_edges(type_vertex_map[TypeName], tc);
       ei != ei_end; ++ei) {

    auto source = boost::source(*ei, tc);
    auto target = boost::target(*ei, tc);
    reachable_nodes.insert(g[target].name);
  }
  return reachable_nodes;
}

string LLVMTypeHierarchy::getVTableEntry(string TypeName, unsigned idx) {
  TypeName = psr::uniformTypeName(TypeName);

  auto iter = vtable_map.find(TypeName);
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

bool LLVMTypeHierarchy::containsType(string TypeName) {
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
    for (auto entry : vtable_map) {
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
