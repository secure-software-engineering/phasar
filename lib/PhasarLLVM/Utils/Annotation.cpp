/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Utils/Annotation.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include <cassert>
#include <memory>

namespace psr {

VarAnnotation::VarAnnotation(const llvm::CallBase *AnnotationCall) noexcept
    : AnnotationCall(AnnotationCall) {
  auto *Callee = AnnotationCall->getCalledFunction();
  assert(Callee && Callee->hasName() &&
         (Callee->getName() == "llvm.var.annotation" ||
          Callee->getName().startswith("llvm.ptr.annotation")));
}

const llvm::Value *VarAnnotation::getValue() const {
  return AnnotationCall->getArgOperand(0);
}

llvm::StringRef VarAnnotation::getAnnotationString() const {
  return retrieveString(1);
}

llvm::StringRef VarAnnotation::getFile() const { return retrieveString(2); }

uint64_t VarAnnotation::getLine() const {
  assert(AnnotationCall->getArgOperand(3)->getType()->isIntegerTy());
  if (const auto *ConstInt =
          llvm::dyn_cast<llvm::ConstantInt>(AnnotationCall->getArgOperand(3))) {
    return ConstInt->getZExtValue();
  }
  return 0;
}

llvm::StringRef VarAnnotation::retrieveString(unsigned Idx) const {
  assert(Idx < AnnotationCall->arg_size());
  const auto *StringPointer = llvm::dyn_cast<llvm::GlobalVariable>(
      llvm::getUnderlyingObject(AnnotationCall->getArgOperand(Idx)));
  if (!StringPointer || !StringPointer->hasInitializer()) {
    return "";
  }

  const auto *ConstData = StringPointer->getInitializer();
  if (const auto *Data = llvm::dyn_cast<llvm::ConstantDataArray>(ConstData)) {
    return Data->getAsCString();
  }

  return "";
}

const llvm::Value *VarAnnotation::getOriginalValueOrOriginalArg(
    const llvm::Value *AnnotatedValue) {

  // check if that values originates from a formal parameter
  for (const auto &User : AnnotatedValue->users()) {
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(User);
        Store && llvm::isa<llvm::Argument>(Store->getValueOperand())) {
      return Store->getValueOperand();
    }
  }
  return AnnotatedValue;
}

GlobalAnnotation::GlobalAnnotation(
    const llvm::ConstantStruct *AnnotationStruct) noexcept
    : AnnotationStruct(AnnotationStruct) {}

const llvm::Function *GlobalAnnotation::getFunction() const {
  const auto *FunCastOp =
      AnnotationStruct->getOperand(0)->stripPointerCastsAndAliases();

  return llvm::dyn_cast<llvm::Function>(FunCastOp);
}

llvm::StringRef GlobalAnnotation::retrieveString(unsigned Idx) const {
  assert(Idx < AnnotationStruct->getNumOperands());
  const auto *StringPointer = llvm::dyn_cast<llvm::GlobalVariable>(
      llvm::getUnderlyingObject(AnnotationStruct->getOperand(Idx)));
  if (!StringPointer || !StringPointer->hasInitializer()) {
    return "";
  }

  const auto *ConstData = StringPointer->getInitializer();
  if (const auto *Data = llvm::dyn_cast<llvm::ConstantDataArray>(ConstData)) {
    return Data->getAsCString();
  }

  return "";
}

llvm::StringRef GlobalAnnotation::getAnnotationString() const {
  return retrieveString(1);
}

llvm::StringRef GlobalAnnotation::getFile() const { return retrieveString(2); }

uint64_t GlobalAnnotation::getLine() const {
  const auto *ConstLineNoOp = AnnotationStruct->getOperand(3);
  if (const auto *LineNo = llvm::dyn_cast<llvm::ConstantInt>(ConstLineNoOp)) {
    return LineNo->getZExtValue();
  }
  return 0;
}

} // namespace psr
