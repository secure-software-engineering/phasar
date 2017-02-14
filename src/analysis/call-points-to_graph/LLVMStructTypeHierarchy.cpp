/*
 * ClassHierarchy.cpp
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#include "LLVMStructTypeHierarchy.hh"


void LLVMStructTypeHierarchy::analyzeModule(const llvm::Module& M)
{
	// TODO check if this is correct!
	cout << "analyzing module ..." << endl;
	auto Structs = M.getIdentifiedStructTypes();
	for (auto Struct : Structs) {
		type_vertex_map[Struct] = boost::add_vertex(Index(index++, Name(Struct)), graph);
	}

	for (auto Struct : Structs) {
		Struct->dump();
		cout << Struct << endl;
		cout << "with subtypes" << endl;
		for (auto Subtype : Struct->subtypes()) {
			Subtype->dump();
			cout << Subtype << endl;
			if (Subtype->isStructTy()) {
				boost::add_edge(type_vertex_map[Struct], type_vertex_map[Subtype], graph);
			}
		}
		cout << "-----" << endl;
	}
}

set<const llvm::Type*> LLVMStructTypeHierarchy::getTransitivelyReachableParentTypes(const llvm::Type* T)
{
	set<const llvm::Type*> reachable_nodes;
	boost::adjacency_list<> tc;
	boost::transitive_closure(graph, tc);
	dfs_tree_edge_visitor vis;
	boost::depth_first_search(tc, boost::visitor(vis).root_vertex(type_vertex_map[T]));
	boost::property_map<digraph_t, boost::vertex_name_t>::type vertex_to_value = get(boost::vertex_name, graph);
	for (auto vertex : collected_vertices) {
		reachable_nodes.insert(vertex_to_value[vertex]);
	}
	return reachable_nodes;
}

vector<const llvm::Function*> LLVMStructTypeHierarchy::constructVTable(const llvm::Type* T, const llvm::Module* M)
{
	vector<const llvm::Function*> VTable;
	for (auto& F : M->functions()) {
		if (F.getArgumentList().size() > 0) {
			if (F.getArgumentList().begin()->getType() == T && F.hasAddressTaken()) {
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
	boost::transitive_closure(graph, tc);

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

bool LLVMStructTypeHierarchy::hasSuperClass(const llvm::Type* T)
{
	return false;
}

bool LLVMStructTypeHierarchy::hasSubClass(const llvm::Type* T)
{
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
	os << "LLVMSructTypeHierarchy graph:\n" << endl;
	boost::print_graph(ch.graph, boost::get(boost::vertex_name, ch.graph));
	return os;
}

void LLVMStructTypeHierarchy::printTransitiveClosure()
{
	boost::adjacency_list<> tc;
	boost::transitive_closure(graph, tc);
	boost::print_graph(tc, boost::get(boost::vertex_name, graph));
}
