/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/ConstantHelper.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Value.h"

bool psr::glca::isConstant(const llvm::Value *Val) {
  // is constantInt, constantFP or constant string
  if (llvm::isa<llvm::ConstantInt>(Val)) { // const int
    return true;
  }
  if (llvm::isa<llvm::ConstantFP>(Val)) { // const fp
    return true;
  }
  if (llvm::isa<llvm::ConstantPointerNull>(Val)) { // NULL
    return true;
  }
  if (const auto *Gep = llvm::dyn_cast<llvm::ConstantExpr>(Val);
      Gep && Val->getType()->isPointerTy() &&
      Val->getType()->getPointerElementType()->isIntegerTy()) {
    // const string
    // val isa GEP
    auto *Op1 = Gep->getOperand(0); // op1 is pointer-operand
    if (auto *Glob = llvm::dyn_cast<llvm::GlobalVariable>(Op1);
        Glob && Glob->hasInitializer()) {
      if (auto *Cdat =
              llvm::dyn_cast<llvm::ConstantDataArray>(Glob->getInitializer())) {
        return true; // it is definitely a const string
      }
    }
  }
  return false;
}
