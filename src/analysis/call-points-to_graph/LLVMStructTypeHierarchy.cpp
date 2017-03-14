/*
 * ClassHierarchy.cpp
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#include "LLVMStructTypeHierarchy.hh"

void LLVMStructTypeHierarchy::analyzeModule(const llvm::Module& M)
{
	cout << "LLVMStructTyoeHierarchy analyzing module ..." << endl;
	auto Structs = M.getIdentifiedStructTypes();
	for (auto Struct : Structs) {
		type_vertex_map[Struct] = boost::add_vertex(g);
		g[type_vertex_map[Struct]].llvmtype = Struct;
		g[type_vertex_map[Struct]].name = Struct->getName().str();
	}
	for (auto Struct : Structs) {
		for (auto Subtype : Struct->subtypes()) {
			if (Subtype->isStructTy()) {
				boost::add_edge(type_vertex_map[Subtype], type_vertex_map[Struct], g);
			}
		}
	}
//	boost::print_graph(g, boost::get(&VertexProperties::name, g));
}

set<const llvm::Type*> LLVMStructTypeHierarchy::getTransitivelyReachableTypes(const llvm::Type* T)
{
	set<const llvm::Type*> reachable_nodes;
	digraph_t tc;
	boost::transitive_closure(g, tc);
	// get all out edges of queried type
	typename boost::graph_traits<digraph_t>::out_edge_iterator ei, ei_end;
	for (tie(ei, ei_end) = boost::out_edges(type_vertex_map[T], tc); ei != ei_end; ++ei) {
		auto source = boost::source(*ei, tc);
		auto target = boost::target(*ei, tc);
		reachable_nodes.insert(g[target].llvmtype);
	}
//	boost::print_graph(tc, boost::get(&VertexProperties::name, g));
//	my_dfs_visitor vis;
//	boost::depth_first_search(tc, boost::visitor(vis).root_vertex(4));
//	boost::property_map<digraph_t, llvm::Type* VertexProperties::*>::type llvmtype = boost::get(&VertexProperties::llvmtype, g);
//	for (auto vertex : *(vis.collected_vertices)) {
//		cout << vertex << endl;
//		reachable_nodes.insert(vertex_to_value[vertex]);
//	}
//	delete vis.collected_vertices;
	return reachable_nodes;
}

vector<const llvm::Function*> LLVMStructTypeHierarchy::constructVTable(const llvm::Type* T, const llvm::Module* M)
{
	vector<const llvm::Function*> VTable;
	for (auto& F : M->functions()) {
		if (F.getArgumentList().size() > 0) {
			if (F.getArgumentList().begin()->getType()->isPointerTy() &&
					F.getArgumentList().begin()->getType()->getPointerElementType() == T &&
					F.hasAddressTaken()) {
				VTable.push_back(&F);
			}
		}
	}
	return VTable;
}

const llvm::Value* trackDefinition(const llvm::Value* v)
{
	cout <<  "trackDefinition" << endl;
	v->dump();
	const llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(v);
	for (auto& op : inst->operands()) {
		if (op) op->dump();
	}
	return v;
}

const llvm::Value* trackLocalValueToAllocaInst(const llvm::Value* V)
{
	V->dump();
	if (llvm::isa<llvm::AllocaInst>(V)) {
		return V;
	}
	const llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(V);
	for (auto& operand : inst->operands()) {
		return trackLocalValueToAllocaInst(operand);
	}
	return nullptr;
}

const llvm::Value* trackLocalValueToNewInst(const llvm::Value* V)
{
	V->dump();
	if (llvm::isa<llvm::BitCastInst>(V)) {
		const llvm::BitCastInst* cast = llvm::dyn_cast<llvm::BitCastInst>(V);
		cout << "LOCAL NEW" << endl;
		for (auto& operand : cast->operands()) {
			operand->dump();
		}
		cast->getType()->dump();
	}
	return nullptr;
}

const llvm::Function* LLVMStructTypeHierarchy::getFunctionFromVirtualCallSite(llvm::Module* M, llvm::ImmutableCallSite ICS)
{
	boost::adjacency_list<> tc;
	boost::transitive_closure(g, tc);

	cout << "CONSTRUCTION SITE" << endl;
	const llvm::Value* val = trackLocalValueToNewInst(ICS->getOperand(0));

	const llvm::Value* v = trackLocalValueToAllocaInst(ICS->getOperand(0));
	const llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(ICS.getCalledValue());
	const llvm::GetElementPtrInst* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(load->getPointerOperand());
	vector<const llvm::Function*> vtable;
	if (containsSubType(v->getType(), ICS->getOperand(0)->getType()) && containsVTable(ICS->getOperand(0)->getType())) {
		cout << "desired" << endl;
		vtable = constructVTable(v->getType(), M);
	} else {
		cout << "undesired" << endl;
		vtable = constructVTable(ICS->getOperand(0)->getType(), M);
	}
	cout << "vtable contents" << endl;
	for (auto entru : vtable) {
		cout << entru->getName().str() << endl;
	}
	cout << "vtable size: " << vtable.size() << endl;
	cout << "index: " << llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(1))->getZExtValue() << endl;
	return vtable[llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(1))->getZExtValue()];
}

bool LLVMStructTypeHierarchy::containsSubType(const llvm::Type* T, const llvm::Type* ST)
{
	for (auto type : T->subtypes()) {
		for (unsigned i =0; i < type->getNumContainedTypes(); ++i) {
			if (ST->getPointerElementType() == type->getContainedType(i))
				return true;
		}
	}
	return false;
}

bool LLVMStructTypeHierarchy::hasSuperType(const llvm::Type* ST, const llvm::Type* T)
{
	cout << "operation not supported, yet" << endl;
	return false;
}

bool LLVMStructTypeHierarchy::hasSubType(const llvm::Type* ST, const llvm::Type* T)
{
	set<vertex_t> result;
	reachability_dfs_visitor vis(result);
	std::vector<boost::default_color_type> color_map(boost::num_vertices(g));
	boost::depth_first_visit(g, type_vertex_map[ST], vis,
				boost::make_iterator_property_map(color_map.begin(), boost::get(boost::vertex_index, g), color_map[0]));
	for (auto vertex : result) {
		if (g[vertex].llvmtype == T) {
			return true;
		}
	}
	return false;
}

bool LLVMStructTypeHierarchy::containsVTable(const llvm::Type* T)
{
	for (auto SubType : T->subtypes()) {
		if (SubType->getNumContainedTypes() > 0) {
			if (SubType->getContainedType(0)->isPointerTy()) {
				if (SubType->getContainedType(0)->getPointerElementType()->isPointerTy()) {
					if (SubType->getContainedType(0)->getPointerElementType()->getPointerElementType()->isFunctionVarArg()) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

ostream& operator<< (ostream& os, const LLVMStructTypeHierarchy& ch)
{
	os << "LLVMSructTypeHierarchy graph:\n";
	boost::print_graph(ch.g, boost::get(&LLVMStructTypeHierarchy::VertexProperties::name, ch.g));
	return os;
}

void LLVMStructTypeHierarchy::printTransitiveClosure()
{
	digraph_t tc;
	boost::transitive_closure(g, tc);
	boost::print_graph(tc, boost::get(&LLVMStructTypeHierarchy::VertexProperties::name, g));
}
