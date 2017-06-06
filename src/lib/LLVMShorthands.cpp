/*
 * LLVMShorthands.cpp
 *
 *  Created on: 15.05.2017
 *      Author: philipp
 */

#include "LLVMShorthands.hh"

bool isFunctionPointer(const llvm::Value* V) noexcept {
	if (V) {
		if (V->getType()->isPointerTy() && V->getType()->getPointerElementType()->isFunctionTy()) {
			return true;
		}
		return false;
	}
	return false;
}

bool matchesSignature(const llvm::Function* F, const llvm::FunctionType* FType) {
	if (F->getArgumentList().size() == FType->getNumParams() && F->getReturnType() == FType->getReturnType()) {
		auto& arglist = F->getArgumentList();
		unsigned i = 0;
		for (auto& arg : arglist) {
			if (arg.getType() != FType->getParamType(i)) {
				return false;
			}
			++i;
		}
		return true;
	}
	return false;
}

string llvmIRToString(const llvm::Value* V) {
	string IRBuffer;
	llvm::raw_string_ostream RSO(IRBuffer);
	V->print(RSO);
	RSO.flush();
	return IRBuffer;
}

vector<const llvm::Value*> globalValuesUsedinFunction(const llvm::Function* F) {
	vector<const llvm::Value*> globals_used;
	for (auto& BB : *F) {
		for (auto& I : BB) {
			for (auto& Op : I.operands()) {
				if (const llvm::GlobalValue* G = llvm::dyn_cast<llvm::GlobalValue>(Op)) {
					globals_used.push_back(G);
				}
			}
		}
	}
	return globals_used;
}

string getMetaDataID(const llvm::Instruction* I) {
	return llvm::cast<llvm::MDString>(I->getMetadata(MetaDataKind)->getOperand(0))->getString().str();
}

const llvm::Argument* getNthFunctionArgument(const llvm::Function* F, unsigned argNo) {
	if (argNo < F->arg_size()) {
		unsigned current = 0;
		for (auto &A : F->args()) {
			if (argNo == current) {
				return &A;
			}
			++current;
		}
	}
	return nullptr;
}
