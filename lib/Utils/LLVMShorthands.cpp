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

#include "llvm/ADT/StringRef.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
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
    } else if (llvm::isa<llvm::CallInst>(V) || llvm::isa<llvm::InvokeInst>(V)) {
      llvm::ImmutableCallSite CS(V);
      return CS.getCalledFunction() != nullptr &&
             HeapAllocationFunctions.count(
                 CS.getCalledFunction()->getName().str());
    }
    return false;
  }
  return false;
}

bool matchesSignature(const llvm::Function *F,
                      const llvm::FunctionType *FType) {
  // FType->print(llvm::outs());
  if (F == nullptr || FType == nullptr) {
    return false;
  }
  if (F->arg_size() == FType->getNumParams() &&
      F->getReturnType() == FType->getReturnType()) {
    unsigned Idx = 0;
    for (const auto &Arg : F->args()) {
      if (Arg.getType() != FType->getParamType(Idx)) {
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

std::string llvmIRToString(const llvm::Value *V) {
  // WARNING: Expensive function, cause is the V->print(RSO)
  //         (20ms on a medium size code (phasar without debug)
  //          80ms on a huge size code (clang without debug),
  //          can be multiplied by times 3 to 5 if passes are enabled)
  std::string IRBuffer;
  llvm::raw_string_ostream RSO(IRBuffer);
  V->print(RSO);
  RSO << " | ID: " << getMetaDataID(V);
  RSO.flush();
  boost::trim_left(IRBuffer);
  return IRBuffer;
}

std::string llvmIRToShortString(const llvm::Value *V) {
  // WARNING: Expensive function, cause is the V->print(RSO)
  //         (20ms on a medium size code (phasar without debug)
  //          80ms on a huge size code (clang without debug),
  //          can be multiplied by times 3 to 5 if passes are enabled)
  std::string IRBuffer;
  llvm::raw_string_ostream RSO(IRBuffer);
  V->print(RSO);
  boost::trim_left(IRBuffer);
  RSO.flush();
  if (IRBuffer.find(", align") != std::string::npos) {
    IRBuffer.erase(IRBuffer.find(", align"));
  } else if (IRBuffer.find(", !") != std::string::npos) {
    IRBuffer.erase(IRBuffer.find(", !"));
  } else if (IRBuffer.size() > 30) {
    IRBuffer.erase(30);
  }
  return IRBuffer + " | ID: " + getMetaDataID(V);
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
  int ArgNr = 0;
  for (const auto &A : Arg->getParent()->args()) {
    if (&A == Arg) {
      return ArgNr;
    }
    ++ArgNr;
  }
  return -1;
}

const llvm::Argument *getNthFunctionArgument(const llvm::Function *F,
                                             unsigned ArgNo) {
  if (ArgNo < F->arg_size()) {
    unsigned Current = 0;
    for (const auto &A : F->args()) {
      if (ArgNo == Current) {
        return &A;
      }
      ++Current;
    }
  }
  return nullptr;
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
      } else {
        ++Current;
      }
    }
  }
  return nullptr;
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

} // namespace psr
