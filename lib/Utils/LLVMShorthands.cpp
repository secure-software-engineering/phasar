/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMShorthands.cpp
 *
 *  Created on: 15.05.2017
 *      Author: philipp
 */

#include <cstdlib>

#include "boost/algorithm/string/trim.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/Config/Configuration.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

namespace psr {

/// Set of functions that allocate heap memory, e.g. new, new[], malloc.
const set<string> HeapAllocationFunctions = {"_Znwm", "_Znam", "malloc",
                                             "calloc", "realloc"};

bool isFunctionPointer(const llvm::Value *V) noexcept {
  if (V) {
    return V->getType()->isPointerTy() &&
           V->getType()->getPointerElementType()->isFunctionTy();
  }
  return false;
}

bool isAllocaInstOrHeapAllocaFunction(const llvm::Value *V) noexcept {
  if (V) {
    if (llvm::isa<llvm::AllocaInst>(V)) {
      return true;
    }
    if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(V)) {
      return CallSite->getCalledFunction() != nullptr &&
             HeapAllocationFunctions.count(
                 CallSite->getCalledFunction()->getName().str());
    }
    return false;
  }
  return false;
}

// For C-style polymorphism we need to check whether a callee candidate would
// be able to sanely access the formal argument.
bool isTypeMatchForFunctionArgument(llvm::Type *Actual, llvm::Type *Formal) {
  // First check for trivial type equality
  if (Actual == Formal) {
    return true;
  }
  // Trivial non-equality, e.g. PointerType and IntegerType
  if (Actual->getTypeID() != Formal->getTypeID()) {
    return false;
  }
  // For PointerType delegate into its element type
  if (llvm::isa<llvm::PointerType>(Actual)) {
    // If formal argument is void *, we can pass anything.
    if (Formal->getPointerElementType()->isIntegerTy(8)) {
      return true;
    }
    return isTypeMatchForFunctionArgument(Actual->getPointerElementType(),
                                          Formal->getPointerElementType());
  }
  // For structs, Formal needs to be somehow contained in Actual.
  if (llvm::isa<llvm::StructType>(Actual)) {
    // Well, we could do sanity checks here, but if the analysed code is insane
    // we would miss callees, so we don't do that.
    return true;
  }
  // Sound fallback if we didn't match until here.
  return false;
}

bool matchesSignature(const llvm::Function *F, const llvm::FunctionType *FType,
                      bool ExactMatch) {
  // FType->print(llvm::outs());
  if (F == nullptr || FType == nullptr) {
    return false;
  }
  if (F->arg_size() == FType->getNumParams() &&
      F->getReturnType() == FType->getReturnType()) {
    unsigned Idx = 0;
    for (const auto &Arg : F->args()) {
      bool TypeMissMatch =
          ExactMatch ? Arg.getType() != FType->getParamType(Idx)
                     : !isTypeMatchForFunctionArgument(FType->getParamType(Idx),
                                                       Arg.getType());
      if (TypeMissMatch) {
        return false;
      }
      ++Idx;
    }
    return true;
  }
  return false;
}

bool matchesSignature(const llvm::FunctionType *FType1,
                      const llvm::FunctionType *FType2) {
  if (FType1 == nullptr || FType2 == nullptr) {
    return false;
  }
  if (FType1->getNumParams() == FType2->getNumParams() &&
      FType1->getReturnType() == FType2->getReturnType()) {
    for (unsigned Idx = 0; Idx < FType1->getNumParams(); ++Idx) {
      if (FType1->getParamType(Idx) != FType2->getParamType(Idx)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

llvm::ModuleSlotTracker &getModuleSlotTrackerFor(const llvm::Value *V) {
  static llvm::SmallDenseMap<const llvm::Module *,
                             std::unique_ptr<llvm::ModuleSlotTracker>, 2>
      ModuleToSlotTracker;
  const auto *M = getModuleFromVal(V);
  auto &Ret = ModuleToSlotTracker[M];
  if (!Ret) {
    Ret = std::make_unique<llvm::ModuleSlotTracker>(M);
  }
  return *Ret;
}

std::string llvmIRToString(const llvm::Value *V) {
  std::string IRBuffer;
  llvm::raw_string_ostream RSO(IRBuffer);
  V->print(RSO, getModuleSlotTrackerFor(V));
  RSO << " | ID: " << getMetaDataID(V);
  RSO.flush();
  boost::trim_left(IRBuffer);
  return IRBuffer;
}

std::string llvmIRToShortString(const llvm::Value *V) {
  std::string IRBuffer;
  llvm::raw_string_ostream RSO(IRBuffer);
  if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V);
      I && !I->getType()->isVoidTy()) {
    V->printAsOperand(RSO, true, getModuleSlotTrackerFor(V));
  } else {
    V->print(RSO, getModuleSlotTrackerFor(V));
  }
  RSO << " | ID: " << getMetaDataID(V);
  RSO.flush();
  boost::trim_left(IRBuffer);
  return IRBuffer;
}

std::vector<const llvm::Value *>
globalValuesUsedinFunction(const llvm::Function *F) {
  std::vector<const llvm::Value *> GlobalsUsed;
  for (const auto &BB : *F) {
    for (const auto &I : BB) {
      for (const auto &Op : I.operands()) {
        if (const llvm::GlobalValue *G =
                llvm::dyn_cast<llvm::GlobalValue>(Op)) {
          GlobalsUsed.push_back(G);
        }
      }
    }
  }
  return GlobalsUsed;
}

std::string getMetaDataID(const llvm::Value *V) {
  if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (auto *Metadata = Inst->getMetadata(PhasarConfig::MetaDataKind())) {
      return llvm::cast<llvm::MDString>(Metadata->getOperand(0))
          ->getString()
          .str();
    }

  } else if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(V)) {
    if (auto *Metadata = GV->getMetadata(PhasarConfig::MetaDataKind())) {
      return llvm::cast<llvm::MDString>(Metadata->getOperand(0))
          ->getString()
          .str();
    }
  } else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    string FName = Arg->getParent()->getName().str();
    string ArgNr = std::to_string(getFunctionArgumentNr(Arg));
    return string(FName + "." + ArgNr);
  }
  return "-1";
}

llvmValueIDLess::llvmValueIDLess() : sless(stringIDLess()) {}

bool llvmValueIDLess::operator()(const llvm::Value *Lhs,
                                 const llvm::Value *Rhs) const {
  std::string LhsId = getMetaDataID(Lhs);
  std::string RhsId = getMetaDataID(Rhs);
  return sless(LhsId, RhsId);
}

int getFunctionArgumentNr(const llvm::Argument *Arg) {
  return int(Arg->getArgNo());
}

const llvm::Argument *getNthFunctionArgument(const llvm::Function *F,
                                             unsigned ArgNo) {
  if (ArgNo >= F->arg_size()) {
    return nullptr;
  }

  return F->getArg(ArgNo);
}

const llvm::Instruction *getLastInstructionOf(const llvm::Function *F) {
  return &F->back().back();
}

const llvm::Instruction *getNthInstruction(const llvm::Function *F,
                                           unsigned Idx) {
  unsigned Current = 1;
  for (const auto &BB : *F) {
    for (const auto &I : BB) {
      if (Current == Idx) {
        return &I;
      }

      ++Current;
    }
  }
  return nullptr;
}

std::vector<const llvm::Instruction *>
getAllExitPoints(const llvm::Function *F) {
  std::vector<const llvm::Instruction *> ret;
  appendAllExitPoints(F, ret);
  return ret;
}

void appendAllExitPoints(const llvm::Function *F,
                         std::vector<const llvm::Instruction *> &ExitPoints) {
  if (!F) {
    return;
  }

  for (const auto &BB : *F) {
    const auto *term = BB.getTerminator();
    assert(term && "Invalid IR: Each BasicBlock must have a terminator "
                   "instruction at the end");
    if (llvm::isa<llvm::ReturnInst>(term) ||
        llvm::isa<llvm::ResumeInst>(term)) {
      ExitPoints.push_back(term);
    }
  }
}

const llvm::Module *getModuleFromVal(const llvm::Value *V) {
  if (const auto *MA = llvm::dyn_cast<llvm::Argument>(V)) {
    return MA->getParent() ? MA->getParent()->getParent() : nullptr;
  }

  if (const auto *BB = llvm::dyn_cast<llvm::BasicBlock>(V)) {
    return BB->getParent() ? BB->getParent()->getParent() : nullptr;
  }

  if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    const llvm::Function *F =
        I->getParent() ? I->getParent()->getParent() : nullptr;
    return F ? F->getParent() : nullptr;
  }
  if (const auto *GV = llvm::dyn_cast<llvm::GlobalValue>(V)) {
    return GV->getParent();
  }
  if (const auto *MAV = llvm::dyn_cast<llvm::MetadataAsValue>(V)) {
    for (const llvm::User *U : MAV->users()) {
      if (llvm::isa<llvm::Instruction>(U)) {
        if (const llvm::Module *M = getModuleFromVal(U)) {
          return M;
        }
      }
    }
  }
  return nullptr;
}

std::string getModuleNameFromVal(const llvm::Value *V) {
  const llvm::Module *M = getModuleFromVal(V);
  return M ? M->getModuleIdentifier() : " ";
}

std::size_t computeModuleHash(llvm::Module *M, bool ConsiderIdentifier) {
  std::string SourceCode;
  if (ConsiderIdentifier) {
    llvm::raw_string_ostream RSO(SourceCode);
    llvm::WriteBitcodeToFile(*M, RSO);
    RSO.flush();
  } else {
    std::string Identifier = M->getModuleIdentifier();
    M->setModuleIdentifier("");
    llvm::raw_string_ostream RSO(SourceCode);
    llvm::WriteBitcodeToFile(*M, RSO);
    RSO.flush();
    M->setModuleIdentifier(Identifier);
  }
  return std::hash<std::string>{}(SourceCode);
}

std::size_t computeModuleHash(const llvm::Module *M) {
  std::string SourceCode;
  llvm::raw_string_ostream RSO(SourceCode);
  llvm::WriteBitcodeToFile(*M, RSO);
  RSO.flush();
  return std::hash<std::string>{}(SourceCode);
}

const llvm::Instruction *getNthTermInstruction(const llvm::Function *F,
                                               unsigned TermInstNo) {
  unsigned Current = 1;
  for (const auto &BB : *F) {
    if (const llvm::Instruction *T = BB.getTerminator()) {
      if (Current == TermInstNo) {
        return T;
      }
      Current++;
    }
  }
  return nullptr;
}

const llvm::StoreInst *getNthStoreInstruction(const llvm::Function *F,
                                              unsigned StoNo) {
  unsigned Current = 1;
  for (const auto &BB : *F) {
    for (const auto &I : BB) {
      if (const auto *S = llvm::dyn_cast<llvm::StoreInst>(&I)) {
        if (Current == StoNo) {
          return S;
        }
        Current++;
      }
    }
  }
  return nullptr;
}

bool isGuardVariable(const llvm::Value *V) {
  if (const auto *ConstBitcast = llvm::dyn_cast<llvm::ConstantExpr>(V);
      ConstBitcast && ConstBitcast->isCast()) {
    V = ConstBitcast->getOperand(0);
  }
  if (const auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(V)) {
    // ZGV is the encoding of "GuardVariable"
    return GV->getName().startswith("_ZGV");
  }
  return false;
}

bool isStaticVariableLazyInitializationBranch(const llvm::BranchInst *Inst) {
  if (Inst->isUnconditional()) {
    return false;
  }

  auto *Condition = Inst->getCondition();

  if (auto *Cmp = llvm::dyn_cast<llvm::ICmpInst>(Condition);
      Cmp && llvm::ICmpInst::isEquality(Cmp->getPredicate())) {
    for (auto *Op : Cmp->operand_values()) {
      if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(Op);
          Load && Load->isAtomic()) {

        if (isGuardVariable(Load->getPointerOperand())) {
          return true;
        }
      } else if (auto *Call = llvm::dyn_cast<llvm::CallBase>(Op)) {
        auto *CalledFunction = Call->getCalledFunction();
        if (CalledFunction &&
            CalledFunction->getName() == "__cxa_guard_acquire") {
          return true;
        }
      }
    }
  }

  return false;
}

bool isVarAnnotationIntrinsic(const llvm::Function *F) {
  static const llvm::StringRef kVarAnnotationName("llvm.var.annotation");
  return F->getName() == kVarAnnotationName;
}

llvm::StringRef getVarAnnotationIntrinsicName(const llvm::CallInst *CallInst) {
  const int kPointerGlobalStringIdx = 1;
  auto *ce = llvm::cast<llvm::ConstantExpr>(
      CallInst->getOperand(kPointerGlobalStringIdx));
  assert(ce != nullptr);
  assert(ce->getOpcode() == llvm::Instruction::GetElementPtr);
  assert(llvm::dyn_cast<llvm::GlobalVariable>(ce->getOperand(0)) != nullptr);

  auto *annoteStr = llvm::dyn_cast<llvm::GlobalVariable>(ce->getOperand(0));
  assert(llvm::dyn_cast<llvm::ConstantDataSequential>(
      annoteStr->getInitializer()));

  auto *data =
      llvm::dyn_cast<llvm::ConstantDataSequential>(annoteStr->getInitializer());

  // getAsCString to get rid of the null-terminator
  assert(data->isCString());
  return data->getAsCString();
}

} // namespace psr
