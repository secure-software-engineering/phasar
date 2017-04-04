/*
 * ValueAnnotationPass.cpp
 *
 *  Created on: 26.01.2017
 *      Author: pdschbrt
 */

#include "ValueAnnotationPass.hh"

size_t ValueAnnotationPass::unique_value_id = 0;

bool  ValueAnnotationPass::runOnModule(llvm::Module& M)
{
	std::string MetaDataKind("ourframework.id");

	std::cout << "running ValueAnnotationPass ...\n";
	for (auto& global : M.globals()) {
		llvm::MDNode* node = llvm::MDNode::get(context, llvm::MDString::get(context, std::to_string(unique_value_id)));
		global.setMetadata(MetaDataKind, node);
//		std::cout << llvm::cast<llvm::MDString>(global.getMetadata(MetaDataKind)->getOperand(0))->getString().str() << std::endl;
		++unique_value_id;
	}

	for (auto structtype : M.getIdentifiedStructTypes()) {
//		llvm::Value* val = llvm::dyn_cast<llvm::Value>(structtype);
//		llvm::MDNode* node = llvm::MDNode::get(context, llvm::MDString::get(context, std::to_string(unique_value_id)));
	}

	for (llvm::Module::iterator MI = M.begin(); MI != M.end(); ++MI) {
		for (llvm::Function::iterator FI = MI->begin(); FI != MI->end(); ++FI) {
			llvm::ilist_iterator<llvm::BasicBlock> BB = FI;
		    for (llvm::BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
		    	llvm::Instruction &I = *BI++;
		    	llvm::MDNode* node = llvm::MDNode::get(context, llvm::MDString::get(context, std::to_string(unique_value_id)));
		    	I.setMetadata(MetaDataKind, node);
//		    	std::cout << llvm::cast<llvm::MDString>(I.getMetadata(MetaDataKind)->getOperand(0))->getString().str() << std::endl;
		    	++unique_value_id;
		    }
		}
	}
	return true;
}

bool ValueAnnotationPass::doInitialization(llvm::Module& M) { return false; }

bool ValueAnnotationPass::doFinalization(llvm::Module& M) {	return false; }

void ValueAnnotationPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const { AU.setPreservesCFG(); }

void ValueAnnotationPass::releaseMemory() {}
