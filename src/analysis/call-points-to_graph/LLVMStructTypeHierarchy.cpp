/*
 * ClassHierarchy.cpp
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#include "LLVMStructTypeHierarchy.hh"

LLVMStructTypeHierarchy::LLVMStructTypeHierarchy(
    const ProjectIRCompiledDB& IRDB) {
  for (auto& module_entry : IRDB.modules) {
    const llvm::Module& M = *(module_entry.second);
    analyzeModule(M);
    reconstructVTable(M);
  }
  // //cout << "transitive closure" << endl;
  // // we have to complete the transitive vtables from derived classes
  // printTransitiveClosure();
  // bidigraph_t tc;
  // boost::transitive_closure(g, tc);
  // typename boost::graph_traits<bidigraph_t>::vertex_iterator vi, vi_end;
  // typename boost::graph_traits<bidigraph_t>::in_edge_iterator ei, ei_end;
  // for (tie(vi, vi_end) = boost::vertices(tc); vi != vi_end; ++vi) {
  //   cout << g[*vi].name << endl;
  //   for (tie(ei, ei_end) = boost::in_edges(*vi, tc); ei != ei_end; ++ei) {
  //     auto source = boost::source(*ei, tc);
  //     //vtable_map[g[*vi].name].addVTable(vtable_map[g[source].name]);
  //     // cout << "is a: " << g[source].name << endl;
  //   }
  // }
}

void LLVMStructTypeHierarchy::reconstructVTable(const llvm::Module& M) {
  const static string vtable_for = "vtable for ";
  llvm::Module& m = const_cast<llvm::Module&>(M);
    for (auto& global : m.globals()) {
      string demangled = cxx_demangle(global.getName().str());
      // cxx_demangle returns an empty string if something goes wrong
      if (demangled == "") continue;
      if (llvm::isa<llvm::Constant>(global) && demangled.find(vtable_for) != demangled.npos) {
        string struct_name = "struct." + demangled.erase(0, vtable_for.size());
        llvm::Constant* initializer = global.getInitializer();
        // ignore 'vtable for __cxxabiv1::__si_class_type_info', also the vtable might be marked as external!
        if (!initializer) continue;
        // check if the vtable is already initialized, then we can skip
        if (vtable_map.find(struct_name) != vtable_map.end()) continue;
        for (unsigned i = 0; i < initializer->getNumOperands(); ++i) {
          if (llvm::ConstantExpr* constant_expr = llvm::dyn_cast<llvm::ConstantExpr>(initializer->getAggregateElement(i))) {
             if (constant_expr->isCast()) {
              if (llvm::Constant* cast = llvm::ConstantExpr::getBitCast(constant_expr, constant_expr->getType())) {
                 if (llvm::Function* vfunc = llvm::dyn_cast<llvm::Function>(cast->getOperand(0))) {
                  // cout << struct_name << ": " << vfunc->getName().str() << endl;
                   vtable_map[struct_name].addEntry(vfunc->getName().str());
                // } else {
                // here is another way to find the type name starting with the vtable
                //   // if it is not a function, it is the corresponding typeinfo
                //   if (llvm::GlobalVariable* typeinfo = llvm::dyn_cast<llvm::GlobalVariable>(constant_expr->getOperand(0))) {
                //     llvm::Constant* typeinfo_init = typeinfo->getInitializer();
                //     if (llvm::ConstantExpr* gep_typename = llvm::dyn_cast<llvm::ConstantExpr>(typeinfo_init->getOperand(1))) {
                //       llvm::Value* name = gep_typename->getOperand(0);
                //       if (llvm::GlobalVariable* glbl_name = llvm::dyn_cast<llvm::GlobalVariable>(name)) {
                //         llvm::ConstantDataArray* literal = llvm::dyn_cast<llvm::ConstantDataArray>(glbl_name->getInitializer());
                //         cout << "TYPE is: " << literal->getAsCString().str() << endl;
                //         string demangled_struct_prefixed_name = "struct." + cxx_demangle(literal->getAsCString().str());
                //         cout << "demangled TYPE is: " << demangled_struct_prefixed_name << endl;
                //       }
                //   }
                // }
                }
              }
            }
          }
        }
      }
    } 
}

void LLVMStructTypeHierarchy::analyzeModule(const llvm::Module& M) {
  auto StructTypes = M.getIdentifiedStructTypes();
  for (auto StructType : StructTypes) {
    // only add a new vertex to the graph if the type is currently unknown!
    if (recognized_struct_types.find(StructType->getName().str()) ==
        recognized_struct_types.end()) {
      type_vertex_map[StructType->getName().str()] = boost::add_vertex(g);
      g[type_vertex_map[StructType->getName().str()]].llvmtype = StructType;
      g[type_vertex_map[StructType->getName().str()]].name =
          StructType->getName().str();
    }
  }
  for (auto StructType : StructTypes) {
    for (auto Subtype : StructType->subtypes()) {
      if (Subtype->isStructTy()) {
        llvm::StructType* StructSubType =
            llvm::dyn_cast<llvm::StructType>(Subtype);
        boost::add_edge(type_vertex_map[StructSubType->getName().str()],
                        type_vertex_map[StructType->getName().str()], g);
      }
    }
  }
  for_each(StructTypes.begin(), StructTypes.end(),
           [this](const llvm::StructType* ST) {
             recognized_struct_types.insert(ST->getName().str());
           });
}

set<string> LLVMStructTypeHierarchy::getTransitivelyReachableTypes(
    string TypeName) {
  set<string> reachable_nodes;
  bidigraph_t tc;
  boost::transitive_closure(g, tc);
  // get all out edges of queried type
  typename boost::graph_traits<bidigraph_t>::out_edge_iterator ei, ei_end;
  for (tie(ei, ei_end) = boost::out_edges(type_vertex_map[TypeName], tc);
       ei != ei_end; ++ei) {
    auto source = boost::source(*ei, tc);
    auto target = boost::target(*ei, tc);
    reachable_nodes.insert(g[target].name);
  }
  //	boost::print_graph(tc, boost::get(&VertexProperties::name, g));
  //	my_dfs_visitor vis;
  //	boost::depth_first_search(tc, boost::visitor(vis).root_vertex(4));
  //	boost::property_map<bidigraph_t, llvm::Type* VertexProperties::*>::type
  // llvmtype = boost::get(&VertexProperties::llvmtype, g);
  //	for (auto vertex : *(vis.collected_vertices)) {
  //		cout << vertex << endl;
  //		reachable_nodes.insert(vertex_to_value[vertex]);
  //	}
  //	delete vis.collected_vertices;
  return reachable_nodes;
}

// const llvm::Function*
// LLVMStructTypeHierarchy::getFunctionFromVirtualCallSite(llvm::Module* M,
// llvm::ImmutableCallSite ICS)
// {
// 	boost::adjacency_list<> tc;
// 	boost::transitive_closure(g, tc);

// 	const llvm::LoadInst* load =
// llvm::dyn_cast<llvm::LoadInst>(ICS.getCalledValue());
// 	const llvm::GetElementPtrInst* gep =
// llvm::dyn_cast<llvm::GetElementPtrInst>(load->getPointerOperand());
// 	vector<const llvm::Function*> vtable;
// 	if (containsSubType(v->getType(), ICS->getOperand(0)->getType()) &&
// containsVTable(ICS->getOperand(0)->getType())) {
// 		cout << "desired" << endl;
// 		vtable = constructVTable(v->getType(), M);
// 	} else {
// 		cout << "undesired" << endl;
// 		vtable = constructVTable(ICS->getOperand(0)->getType(), M);
// 	}
// 	cout << "vtable contents" << endl;
// 	for (auto entru : vtable) {
// 		cout << entru->getName().str() << endl;
// 	}
// 	cout << "vtable size: " << vtable.size() << endl;
// 	cout << "index: " <<
// llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(1))->getZExtValue() <<
// endl;
// 	return
// vtable[llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(1))->getZExtValue()];
// }

string LLVMStructTypeHierarchy::getVTableEntry(string TypeName, unsigned idx) {
  auto iter = vtable_map.find(TypeName);
  if (iter != vtable_map.end()) {
    return iter->second.getFunctionByIdx(idx);
  }
  return "";
}

bool LLVMStructTypeHierarchy::hasSuperType(string TypeName,
                                           string SuperTypeName) {
  cout << "NOT SUPPORTED YET" << endl;
  return false;
}

bool LLVMStructTypeHierarchy::hasSubType(string TypeName, string SubTypeName) {
  auto reachable_types = getTransitivelyReachableTypes(TypeName);
  return reachable_types.find(SubTypeName) != reachable_types.end();
}

bool LLVMStructTypeHierarchy::containsVTable(string TypeName) {
  auto iter = vtable_map.find(TypeName);
  return iter != vtable_map.end();
}

void LLVMStructTypeHierarchy::print() {
  cout << "LLVMSructTypeHierarchy graph:\n";
  boost::print_graph(
      g, boost::get(&LLVMStructTypeHierarchy::VertexProperties::name, g));
  cout << "\nVTables:\n";
  for (auto entry : vtable_map) {
    cout << entry.first << " contains\n" << entry.second << endl;
  }
}

void LLVMStructTypeHierarchy::printTransitiveClosure() {
  bidigraph_t tc;
  boost::transitive_closure(g, tc);
  boost::print_graph(
      tc, boost::get(&LLVMStructTypeHierarchy::VertexProperties::name, g));
}
