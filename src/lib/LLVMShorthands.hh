/*
 * LLVMShorthands.hh
 *
 *  Created on: 15.05.2017
 *      Author: philipp
 */

#ifndef SRC_LIB_LLVMSHORTHANDS_HH_
#define SRC_LIB_LLVMSHORTHANDS_HH_

#include "../utils/Configuration.hh"
#include <string>
#include <vector>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
using namespace std;

bool isFunctionPointer(const llvm::Value* V) noexcept;

bool matchesSignature(const llvm::Function* F, const llvm::FunctionType* FType);

string llvmIRToString(const llvm::Value* V);

vector<const llvm::Value*> globalValuesUsedinFunction(const llvm::Function* F);

string getMetaDataID(const llvm::Instruction*);

const llvm::Argument* getNthFunctionArgument(const llvm::Function* F, unsigned argNo);

#endif /* SRC_LIB_LLVMSHORTHANDS_HH_ */
