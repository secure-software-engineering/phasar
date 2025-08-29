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
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
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
      const auto *AllocVarArg = getVaListTagOrNull(*CalleeFun);
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
  bool DeclarationOnlyCalleeFound = false;
  for (const auto *Callee : Callees) {
    std::string DemangledFname = llvm::demangle(Callee->getName().str());
    // Generate the return value of factory functions from zero value
    if (isFactoryFunction(DemangledFname)) {
      return this->generateFromZero(CS);
    }

    DeclarationOnlyCalleeFound |= Callee->isDeclaration();

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
  if (!DeclarationOnlyCalleeFound) {
    return killFlowIf(
        [](d_t Source) { return llvm::isa<llvm::Constant>(Source); });
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
  if (const auto *StructTy = llvm::dyn_cast<llvm::StructType>(Ty);
      StructTy && StructTy->hasName()) {
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
  // TODO:
  // - determine if general case is even needed anymore, or if we only need
  // the general case
  // - Can I use stripPointerTypes() for all cases below (Alloca, etc)?
  // - How does AllocaInst, LoadInst, etc work under the hood?
  //
  //
  // - Dwarf Tags seem to be ill fit for what I am trying to do here. What other
  // info can I use?
  //
  //
  // Run
  // ./unittests/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETSAnalysisFileIOTest
  // for tests, make sure they're compiled with debug info!

  if (V->getType()->isPointerTy()) {
    if (const auto *DITy = getVarTypeFromIR(V)) {
      // llvm::outs() << "DITy is: " << llvm::dwarf::TagString(DITy->getTag())
      //              << "\n";
      if (const auto *BaseDITy = stripPointerTypes(DITy)) {
        // llvm::outs() << "BaseDITy is: "
        //              << llvm::dwarf::TagString(BaseDITy->getTag()) << "\n";

        if (const auto &Operand = BaseDITy->getOperand(0)) {
          if (const auto *OpType = Operand.get()) {
            return isTypeOfInterest(OpType);
          }
          // llvm::outs() << *(BaseDITy->getOperand(0)) << "\n";
          // return isTypeOfInterest(Operand);
          // if (llvm::isa<llvm::DIFile>(BaseDITy->getOperand(0))) {
          //   llvm::outs() << "Is a DIFile!!!\n";
          // }
        }
        // return isTypeOfInterest(BaseDITy->getTag());
      }

      return false;
    }

    return false;
  }

  return false;

#if false
  // General case
  if (V->getType()->isPointerTy() && !V->getType()->isOpaquePointerTy()) {
    if (hasMatchingTypeName(V->getType()->getNonOpaquePointerElementType())) {
      return true;
    }
    // fallthrough
  }

  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (Alloca->getAllocatedType()->isOpaquePointerTy() ||
          hasMatchingTypeName(
              Alloca->getAllocatedType()->getNonOpaquePointerElementType())) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(V)) {
    if (Load->getType()->isPointerTy()) {
      if (Load->getType()->isOpaquePointerTy() ||
          hasMatchingTypeName(
              Load->getType()->getNonOpaquePointerElementType())) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(V)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (Store->getValueOperand()->getType()->isOpaquePointerTy() ||
          hasMatchingTypeName(Store->getValueOperand()
                                  ->getType()
                                  ->getNonOpaquePointerElementType())) {
        return true;
      }
    }
    return false;
  }
  return false;
#endif
#if false
  if (const auto *DITy = getVarTypeFromIR(V)) {

    llvm::outs() << "------------------------------------------------\n";

    if (DITy) {
      llvm::outs() << "DITy: exists" << "\n";
      if (DITy->getTag()) {
        llvm::outs() << "DITy: had tag" << "\n";
        llvm::outs() << "tag was: " << DITy->getTag() << "\n";
        llvm::outs() << "TagString: " << llvm::dwarf::TagString(DITy->getTag())
                     << "\n";
      } else {
        llvm::outs() << "DITy: not tag, sadly" << "\n";
      }
    } else {
      llvm::outs() << "DITy: was nullptr" << "\n";
    }

    const auto *BaseOfDITy = psr::stripPointerTypes(DITy);

    if (BaseOfDITy) {
      llvm::outs() << "BaseOfDITy: exists" << "\n";

      if (BaseOfDITy->getTag()) {
        llvm::outs() << "BaseOfDITy: had tag" << "\n";
        llvm::outs() << "tag was: " << BaseOfDITy->getTag() << "\n";
        llvm::outs() << "TagString: "
                     << llvm::dwarf::TagString(BaseOfDITy->getTag()) << "\n";
      } else {
        llvm::outs() << "BaseOfDITy: not tag, sadly" << "\n";
      }
    } else {
      llvm::outs() << "BaseOfDITy: was nullptr" << "\n";
    }

    // General case
    if (DITy->getTag() == llvm::dwarf::DW_TAG_structure_type) {
      // if (hasMatchingType(DITy)) {
      //   return true;
      // }
    }

    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
      llvm::outs() << "Was AllocaInst\n";

      if (Alloca->getAllocatedType()->isPointerTy()) {
        if (Alloca->getAllocatedType()->isOpaquePointerTy() ||
            hasMatchingTypeName(
                Alloca->getAllocatedType()->getNonOpaquePointerElementType())) {
          return true;
        }
      }
      return false;
    }

    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(V)) {
      llvm::outs() << "Was LoadInst\n";
      if (Load->getType()->isPointerTy()) {
        if (Load->getType()->isOpaquePointerTy() ||
            hasMatchingTypeName(
                Load->getType()->getNonOpaquePointerElementType())) {
          return true;
        }
      }
      return false;
    }
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(V)) {
      llvm::outs() << "Was StoreInst\n";
      if (Store->getValueOperand()->getType()->isPointerTy()) {
        if (Store->getValueOperand()->getType()->isOpaquePointerTy() ||
            hasMatchingTypeName(Store->getValueOperand()
                                    ->getType()
                                    ->getNonOpaquePointerElementType())) {
          return true;
        }
      }
      return false;
    }

    return false;

#if false
  // General case
  if (V->getType()->isPointerTy() && !V->getType()->isOpaquePointerTy()) {
    if (hasMatchingTypeName(V->getType()->getNonOpaquePointerElementType())) {
      return true;
    }
    // fallthrough
  }

  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (Alloca->getAllocatedType()->isOpaquePointerTy() ||
          hasMatchingTypeName(
              Alloca->getAllocatedType()->getNonOpaquePointerElementType())) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(V)) {
    if (Load->getType()->isPointerTy()) {
      if (Load->getType()->isOpaquePointerTy() ||
          hasMatchingTypeName(
              Load->getType()->getNonOpaquePointerElementType())) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(V)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (Store->getValueOperand()->getType()->isOpaquePointerTy() ||
          hasMatchingTypeName(Store->getValueOperand()
                                  ->getType()
                                  ->getNonOpaquePointerElementType())) {
        return true;
      }
    }
    return false;
  }
  return false;
#endif
#endif
}
} // namespace psr::detail
