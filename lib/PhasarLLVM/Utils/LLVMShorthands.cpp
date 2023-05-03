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

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

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
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "boost/algorithm/string/trim.hpp"

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <system_error>

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

bool isIntegerLikeType(const llvm::Type *T) noexcept {
  if (const auto *StructType = llvm::dyn_cast<llvm::StructType>(T)) {
    return StructType->isPacked() && StructType->elements().size() == 1 &&
           StructType->getElementType(0)->isIntegerTy();
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
  const auto *M = getModuleFromVal(V);
  return ModulesToSlotTracker::getSlotTrackerForModule(M);
}

std::string llvmIRToString(const llvm::Value *V) {
  if (!V) {
    return "<null>";
  }

  std::string IRBuffer;
  llvm::raw_string_ostream RSO(IRBuffer);
  V->print(RSO, getModuleSlotTrackerFor(V));
  RSO << " | ID: " << getMetaDataID(V);
  RSO.flush();
  return llvm::StringRef(IRBuffer).ltrim().str();
}

std::string llvmIRToStableString(const llvm::Value *V) {
  if (!V) {
    return "<null>";
  }
  std::string IRBuffer;
  llvm::raw_string_ostream RSO(IRBuffer);
  V->print(RSO, getModuleSlotTrackerFor(V));
  RSO.flush();

  auto IRBufferRef = llvm::StringRef(IRBuffer).ltrim();

  if (auto Meta = IRBufferRef.find_first_of("!#");
      Meta != llvm::StringRef::npos) {
    IRBufferRef = IRBufferRef.slice(0, Meta).rtrim();

    assert(!IRBufferRef.empty());
    IRBufferRef.consume_back(",");
  }

  IRBuffer = IRBufferRef.str();

  IRBuffer.append(" | ID: ");
  IRBuffer.append(getMetaDataID(V));

  return IRBuffer;
}

std::string llvmIRToShortString(const llvm::Value *V) {
  if (!V) {
    return "<null>";
  }
  std::string IRBuffer;
  llvm::raw_string_ostream RSO(IRBuffer);
  if (const auto *I = llvm::dyn_cast<llvm::Instruction>(V);
      I && !I->getType()->isVoidTy()) {
    V->printAsOperand(RSO, true, getModuleSlotTrackerFor(V));
  } else if (const auto *F = llvm::dyn_cast<llvm::Function>(V)) {
    RSO << F->getName();
  } else {
    V->print(RSO, getModuleSlotTrackerFor(V));
  }
  RSO << " | ID: " << getMetaDataID(V);
  RSO.flush();
  return llvm::StringRef(IRBuffer).ltrim().str();
}

std::string llvmTypeToString(const llvm::Type *Ty, bool Shorten) {
  if (!Ty) {
    return "<null>";
  }
  if (Shorten) {
    if (const auto *StructTy = llvm::dyn_cast<llvm::StructType>(Ty);
        StructTy && StructTy->hasName()) {
      return StructTy->getName().str();
    }
  }

  std::string IRBuffer;
  llvm::raw_string_ostream RSO(IRBuffer);
  Ty->print(RSO, false, Shorten);
  return IRBuffer;
}

void dumpIRValue(const llvm::Value *V) {
  llvm::outs() << llvmIRToString(V) << '\n';
}
void dumpIRValue(const llvm::Instruction *V) {
  llvm::outs() << llvmIRToString(V) << '\n';
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

bool LLVMValueIDLess::operator()(const llvm::Value *Lhs,
                                 const llvm::Value *Rhs) const {
  std::string LhsId = getMetaDataID(Lhs);
  std::string RhsId = getMetaDataID(Rhs);
  return StringIDLess{}(LhsId, RhsId);
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

llvm::SmallVector<const llvm::Instruction *, 2>
getAllExitPoints(const llvm::Function *F) {
  llvm::SmallVector<const llvm::Instruction *, 2> Ret;
  appendAllExitPoints(F, Ret);
  return Ret;
}

void appendAllExitPoints(
    const llvm::Function *F,
    llvm::SmallVectorImpl<const llvm::Instruction *> &ExitPoints) {
  if (!F) {
    return;
  }

  for (const auto &BB : *F) {
    const auto *Term = BB.getTerminator();
    assert(Term && "Invalid IR: Each BasicBlock must have a terminator "
                   "instruction at the end");
    if (llvm::isa<llvm::ReturnInst>(Term) ||
        llvm::isa<llvm::ResumeInst>(Term)) {
      ExitPoints.push_back(Term);
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
  static constexpr llvm::StringLiteral KVarAnnotationName(
      "llvm.var.annotation");
  return F->getName() == KVarAnnotationName;
}

llvm::StringRef getVarAnnotationIntrinsicName(const llvm::CallInst *CallInst) {
  const int KPointerGlobalStringIdx = 1;
  auto *CE = llvm::cast<llvm::ConstantExpr>(
      CallInst->getOperand(KPointerGlobalStringIdx));
  assert(CE != nullptr);
  assert(CE->getOpcode() == llvm::Instruction::GetElementPtr);
  assert(llvm::dyn_cast<llvm::GlobalVariable>(CE->getOperand(0)) != nullptr);

  auto *AnnoteStr = llvm::dyn_cast<llvm::GlobalVariable>(CE->getOperand(0));
  assert(AnnoteStr != nullptr && llvm::dyn_cast<llvm::ConstantDataSequential>(
                                     AnnoteStr->getInitializer()));

  auto *Data =
      llvm::dyn_cast<llvm::ConstantDataSequential>(AnnoteStr->getInitializer());

  // getAsCString to get rid of the null-terminator
  assert(Data->isCString());
  return Data->getAsCString();
}

struct PhasarModuleSlotTrackerWrapper {
  PhasarModuleSlotTrackerWrapper(const llvm::Module *M) : MST(M) {}

  llvm::ModuleSlotTracker MST;
  size_t RefCount = 0;
};

static llvm::SmallDenseMap<const llvm::Module *,
                           std::unique_ptr<PhasarModuleSlotTrackerWrapper>, 2>
    MToST{};

static std::mutex MSTMx;

llvm::ModuleSlotTracker &
ModulesToSlotTracker::getSlotTrackerForModule(const llvm::Module *M) {
  std::lock_guard Lck(MSTMx);

  auto &Ret = MToST[M];
  if (M == nullptr && Ret == nullptr) {
    Ret = std::make_unique<PhasarModuleSlotTrackerWrapper>(M);
    Ret->RefCount++;
  }
  assert(Ret != nullptr && "no ModuleSlotTracker instance for module cached");
  return Ret->MST;
}

void ModulesToSlotTracker::setMSTForModule(const llvm::Module *M) {
  std::lock_guard Lck(MSTMx);

  auto [It, Inserted] = MToST.try_emplace(M, nullptr);
  if (Inserted) {
    It->second = std::make_unique<PhasarModuleSlotTrackerWrapper>(M);
  }
  It->second->RefCount++;
}

void ModulesToSlotTracker::updateMSTForModule(const llvm::Module *Module) {
  std::lock_guard Lck(MSTMx);
  auto It = MToST.find(Module);
  if (It == MToST.end()) {
    llvm::report_fatal_error(
        "Can only update an existing ModuleSlotTracker. There is no MST "
        "registered for the current module!");
  }
  std::destroy_at(It->second.get());
  new (It->second.get()) llvm::ModuleSlotTracker(Module);
}

void ModulesToSlotTracker::deleteMSTForModule(const llvm::Module *M) {
  std::lock_guard Lck(MSTMx);

  auto It = MToST.find(M);
  if (It == MToST.end()) {
    return;
  }

  if (--It->second->RefCount == 0) {
    MToST.erase(It);
  }
}

} // namespace psr
