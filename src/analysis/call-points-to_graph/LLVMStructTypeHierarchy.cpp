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
  return reachable_nodes;
}

string LLVMStructTypeHierarchy::getVTableEntry(string TypeName, unsigned idx) {
  auto iter = vtable_map.find(TypeName);
  if (iter != vtable_map.end()) {
    return iter->second.getFunctionByIdx(idx);
  }
  return "";
}

VTable LLVMStructTypeHierarchy::getVTable(string TypeName) {
  return vtable_map[TypeName];
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

bool LLVMStructTypeHierarchy::containsType(string TypeName) {
  return recognized_struct_types.count(TypeName);
}

void LLVMStructTypeHierarchy::print() {
  cout << "LLVMSructTypeHierarchy graph:\n";
  boost::print_graph(
      g, boost::get(&LLVMStructTypeHierarchy::VertexProperties::name, g));
  cout << "\nVTables:\n";
  if (vtable_map.empty()) {
  	cout << "EMPTY\n";
  } else {
  	for (auto entry : vtable_map) {
    	cout << entry.first << " contains\n" << entry.second << endl;
  	}
  }
}

void LLVMStructTypeHierarchy::printAsDot(const string& path) {
	ofstream ofs(path);
	boost::write_graphviz(ofs, g, boost::make_label_writer(boost::get(&VertexProperties::name, g)));
}

void LLVMStructTypeHierarchy::printTransitiveClosure() {
  bidigraph_t tc;
  boost::transitive_closure(g, tc);
  boost::print_graph(
      tc, boost::get(&LLVMStructTypeHierarchy::VertexProperties::name, g));
}

void LLVMStructTypeHierarchy::exportPATBCJSON() {
		cout << "LLVMStructTypeHierarchy::exportPATBCJSON()\n";
}
