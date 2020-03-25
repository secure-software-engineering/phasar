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

#include "boost/algorithm/string/trim.hpp"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"

#include "phasar/Config/Configuration.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "stdlib.h"

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

SpecialMemberFunctionTy specialMemberFunctionType(const std::string &s) {
  // test if Codes for Constructors, Destructors or operator= are in string
  static const std::map<std::string, SpecialMemberFunctionTy> codes{
      {"C1", SpecialMemberFunctionTy::CTOR},
      {"C2", SpecialMemberFunctionTy::CTOR},
      {"C3", SpecialMemberFunctionTy::CTOR},
      {"D0", SpecialMemberFunctionTy::DTOR},
      {"D1", SpecialMemberFunctionTy::DTOR},
      {"D2", SpecialMemberFunctionTy::DTOR},
      {"aSERKS_", SpecialMemberFunctionTy::CPASSIGNOP},
      {"aSEOS_", SpecialMemberFunctionTy::MVASSIGNOP}};
  std::vector<std::pair<std::size_t, SpecialMemberFunctionTy>> found;
  std::size_t blacklist = 0;
  auto it = codes.begin();
  while (it != codes.end()) {
    if (std::size_t index = s.find(it->first, blacklist)) {
      if (index != std::string::npos) {
        found.push_back(std::make_pair(index, it->second));
        blacklist = index + 1;
      } else {
        ++it;
        blacklist = 0;
      }
    }
  }
  if (found.empty()) {
    return SpecialMemberFunctionTy::NONE;
  }

  // test if codes are in function name or type information
  bool noName = true;
  for (auto index : found) {
    for (auto c = s.begin(); c < s.begin() + index.first; ++c) {
      if (isdigit(*c)) {
        short i = 0;
        while (isdigit(*(c + i))) {
          ++i;
        }
        std::string st(c, c + i);
        if (index.first <= std::distance(s.begin(), c) + stoul(st)) {
          noName = false;
          break;
        } else {
          c = c + *c;
        }
      }
    }
    if (noName) {
      return index.second;
    } else {
      noName = true;
    }
  }
  return SpecialMemberFunctionTy::NONE;
}

SpecialMemberFunctionTy specialMemberFunctionType(const llvm::StringRef &sr) {
  return specialMemberFunctionType(sr.str());
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
  if (F == nullptr || FType == nullptr)
    return false;
  if (F->arg_size() == FType->getNumParams() &&
      F->getReturnType() == FType->getReturnType()) {
    unsigned i = 0;
    for (auto &arg : F->args()) {
      if (arg.getType() != FType->getParamType(i)) {
        return false;
      }
      ++i;
    }
    return true;
  }
  return false;
}

bool matchesSignature(const llvm::FunctionType *FType1,
                      const llvm::FunctionType *FType2) {
  if (FType1 == nullptr || FType2 == nullptr)
    return false;
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
  std::vector<const llvm::Value *> globals_used;
  for (auto &BB : *F) {
    for (auto &I : BB) {
      for (auto &Op : I.operands()) {
        if (const llvm::GlobalValue *G =
                llvm::dyn_cast<llvm::GlobalValue>(Op)) {
          globals_used.push_back(G);
        }
      }
    }
  }
  return globals_used;
}

std::string getMetaDataID(const llvm::Value *V) {
  if (auto Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
    if (auto metaData = Inst->getMetadata(PhasarConfig::MetaDataKind())) {
      return llvm::cast<llvm::MDString>(metaData->getOperand(0))
          ->getString()
          .str();
    }

  } else if (auto GV = llvm::dyn_cast<llvm::GlobalVariable>(V)) {
    if (auto metaData = GV->getMetadata(PhasarConfig::MetaDataKind())) {
      return llvm::cast<llvm::MDString>(metaData->getOperand(0))
          ->getString()
          .str();
    }
  } else if (auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    string FName = Arg->getParent()->getName().str();
    string ArgNr = to_string(getFunctionArgumentNr(Arg));
    return string(FName + "." + ArgNr);
  }
  return "-1";
}

llvmValueIDLess::llvmValueIDLess() : sless(stringIDLess()) {}

bool llvmValueIDLess::operator()(const llvm::Value *lhs,
                                 const llvm::Value *rhs) const {
  std::string lhs_id = getMetaDataID(lhs);
  std::string rhs_id = getMetaDataID(rhs);
  return sless(lhs_id, rhs_id);
}

int getFunctionArgumentNr(const llvm::Argument *Arg) {
  int ArgNr = 0;
  for (auto &A : Arg->getParent()->args()) {
    if (&A == Arg) {
      return ArgNr;
    }
    ++ArgNr;
  }
  return -1;
}

const llvm::Argument *getNthFunctionArgument(const llvm::Function *F,
                                             unsigned argNo) {
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

const llvm::Instruction *getNthInstruction(const llvm::Function *F,
                                           unsigned idx) {
  unsigned i = 1;
  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (i == idx) {
        return &I;
      } else {
        ++i;
      }
    }
  }
  return nullptr;
}

const llvm::Module *getModuleFromVal(const llvm::Value *V) {
  if (const llvm::Argument *MA = llvm::dyn_cast<llvm::Argument>(V))
    return MA->getParent() ? MA->getParent()->getParent() : nullptr;

  if (const llvm::BasicBlock *BB = llvm::dyn_cast<llvm::BasicBlock>(V))
    return BB->getParent() ? BB->getParent()->getParent() : nullptr;

  if (const llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(V)) {
    const llvm::Function *F =
        I->getParent() ? I->getParent()->getParent() : nullptr;
    return F ? F->getParent() : nullptr;
  }
  if (const llvm::GlobalValue *GV = llvm::dyn_cast<llvm::GlobalValue>(V))
    return GV->getParent();
  if (const auto *MAV = llvm::dyn_cast<llvm::MetadataAsValue>(V)) {
    for (const llvm::User *U : MAV->users())
      if (llvm::isa<llvm::Instruction>(U))
        if (const llvm::Module *M = getModuleFromVal(U))
          return M;
    return nullptr;
  }
  return nullptr;
}

const std::string getModuleNameFromVal(const llvm::Value *V) {
  const llvm::Module *M = getModuleFromVal(V);
  return M ? M->getModuleIdentifier() : " ";
}

std::size_t computeModuleHash(llvm::Module *M, bool considerIdentifier) {
  std::string SourceCode;
  if (considerIdentifier) {
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
                                               unsigned termInstNo) {
  unsigned current = 1;
  for (auto &BB : *F) {
    if (const llvm::Instruction *T = BB.getTerminator()) {
      if (current == termInstNo) {
        return T;
      }
      current++;
    }
  }
  return nullptr;
}

const llvm::StoreInst *getNthStoreInstruction(const llvm::Function *F,
                                              unsigned stoNo) {
  unsigned current = 1;
  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (const llvm::StoreInst *S = llvm::dyn_cast<llvm::StoreInst>(&I)) {
        if (current == stoNo) {
          return S;
        }
        current++;
      }
    }
  }
  return nullptr;
}

} // namespace psr
