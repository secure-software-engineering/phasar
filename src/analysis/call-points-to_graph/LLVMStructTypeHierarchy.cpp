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
}

void LLVMStructTypeHierarchy::reconstructVTable(const llvm::Module& M) {
  for (auto& function : M.functions()) {
    if (function.getArgumentList().size() > 0 && function.hasAddressTaken() &&
        function.getArgumentList().begin()->getType()->isPointerTy() &&
        function.getArgumentList()
            .begin()
            ->getType()
            ->getPointerElementType()
            ->isStructTy()) {
      llvm::Type* T = function.getArgumentList()
                          .begin()
                          ->getType()
                          ->getPointerElementType();
      llvm::StructType* ST = llvm::dyn_cast<llvm::StructType>(T);
      vtable_map[ST->getName().str()].addEntry(function.getName().str());
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
  digraph_t tc;
  boost::transitive_closure(g, tc);
  // get all out edges of queried type
  typename boost::graph_traits<digraph_t>::out_edge_iterator ei, ei_end;
  for (tie(ei, ei_end) = boost::out_edges(type_vertex_map[TypeName], tc);
       ei != ei_end; ++ei) {
    auto source = boost::source(*ei, tc);
    auto target = boost::target(*ei, tc);
    reachable_nodes.insert(g[target].name);
  }
  //	boost::print_graph(tc, boost::get(&VertexProperties::name, g));
  //	my_dfs_visitor vis;
  //	boost::depth_first_search(tc, boost::visitor(vis).root_vertex(4));
  //	boost::property_map<digraph_t, llvm::Type* VertexProperties::*>::type
  //llvmtype = boost::get(&VertexProperties::llvmtype, g);
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
    return iter->second.getFunctionByEntry(idx);
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
  cout << "VTables:\n";
  for (auto entry : vtable_map) {
    cout << entry.first << " contains\n" << entry.second << endl;
  }
}

void LLVMStructTypeHierarchy::printTransitiveClosure() {
  digraph_t tc;
  boost::transitive_closure(g, tc);
  boost::print_graph(
      tc, boost::get(&LLVMStructTypeHierarchy::VertexProperties::name, g));
}
