/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <utility>

namespace psr::detail {

auto IDETypeStateAnalysisBase::getNormalFlowFunction(n_t Curr, n_t /*Succ*/)
    -> FlowFunctionPtrType {
  // Check if Alloca's type matches the target type. If so, generate from zero
  // value.
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Curr)) {
    if (hasMatchingType(Alloca)) {
      return this->generateFromZero(Alloca);
    }
  }
  // Check load instructions for target type. Generate from the loaded value
  // and kill the load instruction if it was generated previously (strong
  // update!).
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    if (hasMatchingType(Load)) {
      return transferFlow(Load, Load->getPointerOperand());
    }
  }
  if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    if (hasMatchingType(Gep->getPointerOperand())) {
      return lambdaFlow([=](d_t Source) -> std::set<d_t> {
        // if (Source == Gep->getPointerOperand()) {
        //  return {Source, Gep};
        //}
        return {Source};
      });
    }
  }
  // Check store instructions for target type. Perform a strong update, i.e.
  // kill the alloca pointed to by the pointer-operand and all alloca's
  // related to the value-operand and then generate them from the
  // value-operand.
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    if (hasMatchingType(Store)) {
      auto RelevantAliasesAndAllocas = getLocalAliasesAndAllocas(
          Store->getPointerOperand(), // pointer- or value operand???
          // Store->getValueOperand(),
          Curr->getFunction()->getName().str());

      RelevantAliasesAndAllocas.insert(Store->getValueOperand());
      return lambdaFlow(
          [Store, AliasesAndAllocas = std::move(RelevantAliasesAndAllocas)](
              d_t Source) -> container_type {
            // We kill all relevant loacal aliases and alloca's
            if (Source == Store->getPointerOperand()) {
              // XXX: later kill must-aliases too
              return {};
            }
            // Generate all local aliases and relevant alloca's from the
            // stored value
            if (Source == Store->getValueOperand()) {
              return AliasesAndAllocas;
            }
            return {Source};
          });
    }
  }
  return identityFlow();
}

auto IDETypeStateAnalysisBase::getCallFlowFunction(n_t CallSite, f_t DestFun)
    -> FlowFunctionPtrType {
  // Kill all data-flow facts if we hit a function of the target API.
  // Those functions are modled within Call-To-Return.
  if (isAPIFunction(llvm::demangle(DestFun->getName().str()))) {
    return killAllFlows();
  }
  // Otherwise, if we have an ordinary function call, we can just use the
  // standard mapping.
  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
    return mapFactsToCallee(Call, DestFun);
  }
  llvm::report_fatal_error("callSite not a CallInst nor a InvokeInst");
}

auto IDETypeStateAnalysisBase::getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                                  n_t ExitStmt, n_t /*RetSite*/)
    -> FlowFunctionPtrType {

  /// TODO: Implement return-POI in LLVMFlowFunctions.h
  return lambdaFlow([this, CalleeFun, CS = llvm::cast<llvm::CallBase>(CallSite),
                     Ret = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt)](
                        d_t Source) -> container_type {
    if (LLVMZeroValue::isLLVMZeroValue(Source)) {
      return {Source};
    }
    container_type Res;
    // Handle C-style varargs functions
    if (CalleeFun->isVarArg() && !CalleeFun->isDeclaration()) {
      const llvm::Instruction *AllocVarArg;
      // Find the allocation of %struct.__va_list_tag
      for (const auto &BB : *CalleeFun) {
        for (const auto &I : BB) {
          if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
            if (Alloc->getAllocatedType()->isArrayTy() &&
                Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
                Alloc->getAllocatedType()
                    ->getArrayElementType()
                    ->isStructTy() &&
                Alloc->getAllocatedType()
                        ->getArrayElementType()
                        ->getStructName() == "struct.__va_list_tag") {
              AllocVarArg = Alloc;
              // TODO break out this nested loop earlier (without goto ;-)
            }
          }
        }
      }
      // Generate the varargs things by using an over-approximation
      if (Source == AllocVarArg) {
        for (unsigned Idx = CalleeFun->arg_size(); Idx < CS->arg_size();
             ++Idx) {
          Res.insert(CS->getArgOperand(Idx));
        }
      }
    }
    // Handle ordinary case
    // Map formal parameter into corresponding actual parameter.
    for (auto [Formal, Actual] : llvm::zip(CalleeFun->args(), CS->args())) {
      if (Source == &Formal) {
        Res.insert(Actual); // corresponding actual
      }
    }

    // Collect the return value
    if (Ret && Source == Ret->getReturnValue()) {
      Res.insert(CS);
    }

    // Collect all relevant alloca's to map into caller context
    {
      container_type RelAllocas;
      for (const auto *Fact : Res) {
        const auto &Allocas = getRelevantAllocas(Fact);
        RelAllocas.insert(Allocas.begin(), Allocas.end());
      }
      Res.insert(RelAllocas.begin(), RelAllocas.end());
    }

    return Res;
  });
}

auto IDETypeStateAnalysisBase::getCallToRetFlowFunction(
    n_t CallSite, n_t /*RetSite*/, llvm::ArrayRef<f_t> Callees)
    -> FlowFunctionPtrType {
  const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
  for (const auto *Callee : Callees) {
    std::string DemangledFname = llvm::demangle(Callee->getName().str());
    // Generate the return value of factory functions from zero value
    if (isFactoryFunction(DemangledFname)) {
      return this->generateFromZero(CS);
    }

    /// XXX: Revisit this:

    // Handle all functions that are not modeld with special semantics.
    // Kill actual parameters of target type and all its aliases
    // and the corresponding alloca(s) as these data-flow facts are
    // (inter-procedurally) propagated via Call- and the corresponding
    // Return-Flow. Otherwise we might propagate facts with not updated
    // states.
    // Alloca's related to the return value of non-api functions will
    // not be killed during call-to-return, since it is not safe to assume
    // that the return value will be used afterwards, i.e. is stored to memory
    // pointed to by related alloca's.
    if (!isAPIFunction(DemangledFname) && !Callee->isDeclaration()) {
      for (const auto &Arg : CS->args()) {
        if (hasMatchingType(Arg)) {
          return killManyFlows(getWMAliasesAndAllocas(Arg.get()));
        }
      }
    }
  }
  return identityFlow();
}

auto IDETypeStateAnalysisBase::getSummaryFlowFunction(n_t /*CallSite*/,
                                                      f_t /*DestFun*/)
    -> FlowFunctionPtrType {
  return nullptr;
}

auto IDETypeStateAnalysisBase::getRelevantAllocas(d_t V) -> container_type {
  if (RelevantAllocaCache.find(V) != RelevantAllocaCache.end()) {
    return RelevantAllocaCache[V];
  }
  auto AliasSet = getWMAliasSet(V);
  container_type RelevantAllocas;
  PHASAR_LOG_LEVEL(DEBUG, "Compute relevant alloca's of " << DToString(V));
  for (const auto *Alias : AliasSet) {
    PHASAR_LOG_LEVEL(DEBUG, "Alias: " << DToString(Alias));
    // Collect the pointer operand of a aliased load instruciton
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Alias)) {
      if (hasMatchingType(Alias)) {
        PHASAR_LOG_LEVEL(
            DEBUG, " -> Alloca: " << DToString(Load->getPointerOperand()));
        RelevantAllocas.insert(Load->getPointerOperand());
      }
    } else {
      // For all other types of aliases, e.g. callsites, function arguments,
      // we check store instructions where thoses aliases are value operands.
      for (const auto *User : Alias->users()) {
        PHASAR_LOG_LEVEL(DEBUG, "  User: " << DToString(User));
        if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(User)) {
          if (hasMatchingType(Store)) {
            PHASAR_LOG_LEVEL(DEBUG, "    -> Alloca: " << DToString(
                                        Store->getPointerOperand()));
            RelevantAllocas.insert(Store->getPointerOperand());
          }
        }
      }
    }
  }
  for (const auto *Alias : AliasSet) {
    RelevantAllocaCache[Alias] = RelevantAllocas;
  }
  return RelevantAllocas;
}

auto IDETypeStateAnalysisBase::getWMAliasSet(d_t V) -> container_type {
  if (AliasCache.find(V) != AliasCache.end()) {
    container_type AliasSet(AliasCache[V].begin(), AliasCache[V].end());
    return AliasSet;
  }
  auto PTS = PT.getAliasSet(V);
  for (const auto *Alias : *PTS) {
    if (hasMatchingType(Alias)) {
      AliasCache[Alias] = *PTS;
    }
  }
  container_type AliasSet(PTS->begin(), PTS->end());
  return AliasSet;
}

auto IDETypeStateAnalysisBase::getWMAliasesAndAllocas(d_t V) -> container_type {
  container_type AliasAndAllocas;
  container_type RelevantAllocas = getRelevantAllocas(V);
  container_type Aliases = getWMAliasSet(V);
  AliasAndAllocas.insert(Aliases.begin(), Aliases.end());
  AliasAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return AliasAndAllocas;
}

auto IDETypeStateAnalysisBase::getLocalAliasesAndAllocas(
    d_t V, llvm::StringRef /*Fname*/) -> container_type {
  container_type AliasAndAllocas;
  container_type RelevantAllocas = getRelevantAllocas(V);
  container_type Aliases; // =
                          // IRDB->getAliasGraph(Fname)->getAliasSet(V);
  for (const auto *Alias : Aliases) {
    if (hasMatchingType(Alias)) {
      AliasAndAllocas.insert(Alias);
    }
  }
  // AliasAndAllocas.insert(Aliases.begin(), Aliases.end());
  AliasAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return AliasAndAllocas;
}

bool IDETypeStateAnalysisBase::hasMatchingTypeName(const llvm::Type *Ty) {
  if (const auto *StructTy = llvm::dyn_cast<llvm::StructType>(Ty)) {
    return isTypeNameOfInterest(StructTy->getName());
  }
  // primitive type
  std::string Str;
  llvm::raw_string_ostream S(Str);
  S << *Ty;
  S.flush();
  return isTypeNameOfInterest(Str);
}

bool IDETypeStateAnalysisBase::hasMatchingType(d_t V) {
  // General case
  if (V->getType()->isPointerTy()) {
    if (hasMatchingTypeName(V->getType()->getPointerElementType())) {
      return true;
    }
  }
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (hasMatchingTypeName(
              Alloca->getAllocatedType()->getPointerElementType())) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(V)) {
    if (Load->getType()->isPointerTy()) {
      if (hasMatchingTypeName(Load->getType()->getPointerElementType())) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(V)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (hasMatchingTypeName(
              Store->getValueOperand()->getType()->getPointerElementType())) {
        return true;
      }
    }
    return false;
  }
  return false;
}

} // namespace psr::detail
