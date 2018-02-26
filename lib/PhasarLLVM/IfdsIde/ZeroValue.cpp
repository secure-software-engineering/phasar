/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ZeroValue.cpp
 *
 *  Created on: 23.05.2017
 *      Author: philipp
 */

#include <phasar/PhasarLLVM/IfdsIde/ZeroValue.h>

const string ZeroValueInternalName("zero_value");
const string ZeroValueInternalModuleName("zero_module");
const unique_ptr<llvm::LLVMContext> ZeroValueCTX(new llvm::LLVMContext);
const unique_ptr<llvm::Module> ZeroValueMod(
    new llvm::Module(ZeroValueInternalModuleName, *ZeroValueCTX));

bool isLLVMZeroValue(const llvm::Value *V) {
  if (V->hasName()) {
    // checks if V's name start with "zero_value"
    return V->getName().str().find(ZeroValueInternalName) == 0;
  }
  return false;
}

ZeroValue::ZeroValue()
    : llvm::GlobalVariable(
          *ZeroValueMod, llvm::Type::getIntNTy(*ZeroValueCTX, 2), true,
          llvm::GlobalValue::LinkageTypes::ExternalLinkage,
          llvm::ConstantInt::get(*ZeroValueCTX, llvm::APInt(/*nbits*/ 2,
                                                            /*value*/ 0,
                                                            /*signed*/ true)),
          ZeroValueInternalName) {
  setAlignment(4);
}

ZeroValue *ZeroValue::getInstance() {
  static ZeroValue *zv = new ZeroValue;
  return zv;
}
