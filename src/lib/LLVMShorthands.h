/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMShorthands.h
 *
 *  Created on: 15.05.2017
 *      Author: philipp
 */

#ifndef SRC_LIB_LLVMSHORTHANDS_H_
#define SRC_LIB_LLVMSHORTHANDS_H_

#include "../config/Configuration.h"
#include <functional>
#include <iostream>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>
using namespace std;

bool isFunctionPointer(const llvm::Value *V) noexcept;

bool matchesSignature(const llvm::Function *F, const llvm::FunctionType *FType);

string llvmIRToString(const llvm::Value *V);

vector<const llvm::Value *> globalValuesUsedinFunction(const llvm::Function *F);

string getMetaDataID(const llvm::Instruction *);

const llvm::Argument *getNthFunctionArgument(const llvm::Function *F,
                                             unsigned argNo);

const llvm::Instruction *getNthInstruction(const llvm::Function *F,
                                           const unsigned idx);

const llvm::Module *getModuleFromVal(const llvm::Value *V);

size_t computeModuleHash(llvm::Module *M, bool considerIdentifier);

#endif /* SRC_LIB_LLVMSHORTHANDS_HH_ */
