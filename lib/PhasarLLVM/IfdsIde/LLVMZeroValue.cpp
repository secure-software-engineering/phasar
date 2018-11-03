/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMZeroValue.cpp
 *
 *  Created on: 23.05.2017
 *      Author: philipp
 */

#include <llvm/IR/Constants.h>

#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>

using namespace psr;
using namespace std;
namespace psr {

const string LLVMZeroValueInternalName("zero_value");
const string LLVMZeroValueInternalModuleName("zero_module");
const unique_ptr<llvm::LLVMContext> LLVMZeroValueCTX(new llvm::LLVMContext);
const unique_ptr<llvm::Module> LLVMZeroValueMod(
    new llvm::Module(LLVMZeroValueInternalModuleName, *LLVMZeroValueCTX));

bool isLLVMZeroValue(const llvm::Value *V) {
  if (V && V->hasName()) {
    // checks if V's name start with "zero_value"
    return V->getName().str().find(LLVMZeroValueInternalName) != string::npos;
  }
  return false;
}

LLVMZeroValue::LLVMZeroValue()
    : llvm::GlobalVariable(*LLVMZeroValueMod,
                           llvm::Type::getIntNTy(*LLVMZeroValueCTX, 2), true,
                           llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                           llvm::ConstantInt::get(*LLVMZeroValueCTX,
                                                  llvm::APInt(/*nbits*/ 2,
                                                              /*value*/ 0,
                                                              /*signed*/ true)),
                           LLVMZeroValueInternalName) {
  setAlignment(4);
}

LLVMZeroValue *LLVMZeroValue::getInstance() {
  static LLVMZeroValue *zv = new LLVMZeroValue;
  return zv;
}

} // namespace psr
