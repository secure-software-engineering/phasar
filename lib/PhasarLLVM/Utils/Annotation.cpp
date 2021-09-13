/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <cassert>
#include <memory>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include "phasar/PhasarLLVM/Utils/Annotation.h"

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
  if (const auto *ConstExpr = llvm::dyn_cast<llvm::ConstantExpr>(
          AnnotationCall->getArgOperand(Idx))) {
    if (ConstExpr->isGEPWithNoNotionalOverIndexing()) {
      if (const auto *GlobalVar =
              llvm::dyn_cast<llvm::GlobalVariable>(ConstExpr->getOperand(0))) {
        if (GlobalVar->hasInitializer()) {
          const auto *ConstData = GlobalVar->getInitializer();
          if (const auto *Data =
                  llvm::dyn_cast<llvm::ConstantDataArray>(ConstData)) {
            return Data->getAsCString();
          }
        }
      }
    }
  }
  return "";
}

const llvm::Value *VarAnnotation::getOriginalValueOrOriginalArg(
    const llvm::Value *AnnotatedValue) {

  if (const auto *BitCast =
          llvm::dyn_cast<llvm::BitCastOperator>(AnnotatedValue)) {
    // this may be already the original value
    const auto *Value = BitCast->getOperand(0);
    // check if that values originates from a formal parameter
    for (const auto &User : Value->users()) {
      if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(User);
          Store && llvm::isa<llvm::Argument>(Store->getValueOperand())) {
        return Store->getValueOperand();
      }
    }
    return Value;
  }
  return nullptr;
}

GlobalAnnotation::GlobalAnnotation(
    const llvm::ConstantStruct *AnnotationStruct) noexcept
    : AnnotationStruct(AnnotationStruct) {}

const llvm::Function *GlobalAnnotation::getFunction() const {
  const auto *FunCastOp = AnnotationStruct->getOperand(0);
  if (const auto *BitCast = llvm::dyn_cast<llvm::BitCastOperator>(FunCastOp)) {
    if (const auto *Fun =
            llvm::dyn_cast<llvm::Function>(BitCast->getOperand(0))) {
      return Fun;
    }
  }
  return nullptr;
}

llvm::StringRef GlobalAnnotation::retrieveString(unsigned Idx) const {
  const auto *AnnotationGepOp = AnnotationStruct->getOperand(Idx);
  if (const auto *ConstExpr =
          llvm::dyn_cast<llvm::ConstantExpr>(AnnotationGepOp)) {
    if (ConstExpr->isGEPWithNoNotionalOverIndexing()) {
      if (const auto *GlobalVar =
              llvm::dyn_cast<llvm::GlobalVariable>(ConstExpr->getOperand(0))) {
        if (GlobalVar->hasInitializer()) {
          const auto *ConstData = GlobalVar->getInitializer();
          if (const auto *Data =
                  llvm::dyn_cast<llvm::ConstantDataArray>(ConstData)) {
            return Data->getAsCString();
          }
        }
      }
    }
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
