/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/CFLAndersAliasAnalysis.h"
#include "llvm/Analysis/CFLSteensAliasAnalysis.h"
#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasedPointsToAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"

using namespace psr;

namespace psr {

static void printResults(llvm::AliasResult AR, bool P, const llvm::Value *V1,
                         const llvm::Value *V2, const llvm::Module *M) {
  if (P) {
    std::string O1;

    std::string O2;
    {
      llvm::raw_string_ostream OS1(O1);

      llvm::raw_string_ostream OS2(O2);
      V1->printAsOperand(OS1, true, M);
      V2->printAsOperand(OS2, true, M);
    }

    if (O2 < O1) {
      std::swap(O1, O2);
    }
    llvm::errs() << "  " << AR << ":\t" << O1 << ", " << O2 << "\n";
  }
}

static inline void printModRefResults(const char *Msg, bool P,
                                      const llvm::Instruction *I,
                                      const llvm::Value *Ptr,
                                      const llvm::Module *M) {
  if (P) {
    llvm::errs() << "  " << Msg << ":  Ptr: ";
    Ptr->printAsOperand(llvm::errs(), true, M);
    llvm::errs() << "\t<->" << *I << '\n';
  }
}

static inline void printModRefResults(const char *Msg, bool P,
                                      const llvm::CallBase *CallA,
                                      const llvm::CallBase *CallB,
                                      const llvm::Module * /*M*/) {
  if (P) {
    llvm::errs() << "  " << Msg << ": " << *CallA << " <-> " << *CallB << '\n';
  }
}

static inline void printLoadStoreResults(llvm::AliasResult AR, bool P,
                                         const llvm::Value *V1,
                                         const llvm::Value *V2,
                                         const llvm::Module * /*M*/) {
  if (P) {
    llvm::errs() << "  " << AR << ": " << *V1 << " <-> " << *V2 << '\n';
  }
}

bool LLVMBasedPointsToAnalysis::hasPointsToInfo(
    const llvm::Function &Fun) const {
  return AAInfos.find(&Fun) != AAInfos.end();
}

void LLVMBasedPointsToAnalysis::computePointsToInfo(llvm::Function &Fun) {
  llvm::PreservedAnalyses PA = FPM.run(Fun, FAM);
  llvm::AAResults &AAR = FAM.getResult<llvm::AAManager>(Fun);
  AAInfos.insert(std::make_pair(&Fun, &AAR));
}

void LLVMBasedPointsToAnalysis::erase(llvm::Function *F) {
  // after we clear all stuff, we need to set it up for the next function-wise
  // analysis
  AAInfos.erase(F);
  FAM.clear(*F, F->getName());
}

void LLVMBasedPointsToAnalysis::clear() {
  AAInfos.clear();
  FAM.clear();
}

LLVMBasedPointsToAnalysis::LLVMBasedPointsToAnalysis(ProjectIRDB &IRDB,
                                                     bool UseLazyEvaluation,
                                                     PointerAnalysisType PATy)
    : PATy(PATy) {
  AA.registerFunctionAnalysis<llvm::BasicAA>();
  switch (PATy) {
  case PointerAnalysisType::CFLAnders:
    AA.registerFunctionAnalysis<llvm::CFLAndersAA>();
    break;
  case PointerAnalysisType::CFLSteens:
    AA.registerFunctionAnalysis<llvm::CFLSteensAA>();
    break;
  default:
    break;
  }
  AA.registerFunctionAnalysis<llvm::TypeBasedAA>();
  FAM.registerPass([&] { return std::move(AA); });
  PB.registerFunctionAnalyses(FAM);
  llvm::FunctionPassManager FPM;
  // Always verify the input.
  FPM.addPass(llvm::VerifierPass());
  if (!UseLazyEvaluation) {
    for (llvm::Module *M : IRDB.getAllModules()) {
      for (auto &F : *M) {
        if (!F.isDeclaration()) {
          computePointsToInfo(F);
        }
      }
    }
  }
}

void LLVMBasedPointsToAnalysis::print(std::ostream &OS) const {
  OS << "Points-to Info:\n";
  for (auto &[Fn, AA] : AAInfos) {
    bool PrintAll = true;
    bool PrintNoAlias = true;
    bool PrintMayAlias = true;
    bool PrintPartialAlias = true;
    bool PrintMustAlias = true;
    bool EvalAAMD = true;
    bool PrintNoModRef = true;
    bool PrintMod = true;
    bool PrintRef = true;
    bool PrintModRef = true;
    bool PrintMust = true;
    bool PrintMustMod = true;
    bool PrintMustRef = true;
    bool PrintMustModRef = true;

    // taken from llvm/Analysis/AliasAnalysisEvaluator.cpp
    const llvm::DataLayout &DL = Fn->getParent()->getDataLayout();

    llvm::SetVector<const llvm::Value *> Pointers;
    llvm::SmallSetVector<const llvm::CallBase *, 16> Calls;
    llvm::SetVector<const llvm::Value *> Loads;
    llvm::SetVector<const llvm::Value *> Stores;

    for (const auto &I : Fn->args()) {
      if (I.getType()->isPointerTy()) { // Add all pointer arguments.
        Pointers.insert(&I);
      }
    }

    for (llvm::const_inst_iterator I = inst_begin(*Fn), E = inst_end(*Fn);
         I != E; ++I) {
      if (I->getType()->isPointerTy()) { // Add all pointer instructions.
        Pointers.insert(&*I);
      }
      if (EvalAAMD && llvm::isa<llvm::LoadInst>(&*I)) {
        Loads.insert(&*I);
      }
      if (EvalAAMD && llvm::isa<llvm::StoreInst>(&*I)) {
        Stores.insert(&*I);
      }
      const llvm::Instruction &Inst = *I;
      if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst)) {
        llvm::Value *Callee = Call->getCalledOperand();
        // Skip actual functions for direct function calls.
        if (!llvm::isa<llvm::Function>(Callee) &&
            isInterestingPointer(Callee)) {
          Pointers.insert(Callee);
        }
        // Consider formals.
        for (const llvm::Use &DataOp : Call->data_ops()) {
          if (isInterestingPointer(DataOp)) {
            Pointers.insert(DataOp);
          }
        }
        Calls.insert(Call);
      } else {
        // Consider all operands.
        for (llvm::Instruction::const_op_iterator OI = Inst.op_begin(),
                                                  OE = Inst.op_end();
             OI != OE; ++OI) {
          if (isInterestingPointer(*OI)) {
            Pointers.insert(*OI);
          }
        }
      }
    }

    if (PrintAll || PrintNoAlias || PrintMayAlias || PrintPartialAlias ||
        PrintMustAlias || PrintNoModRef || PrintMod || PrintRef ||
        PrintModRef) {
      OS << "Function: " << Fn->getName().str() << ": " << Pointers.size()
         << " pointers, " << Calls.size() << " call sites\n";
    }

    // iterate over the worklist, and run the full (n^2)/2 disambiguations
    for (auto I1 = Pointers.begin(), E = Pointers.end(); I1 != E; ++I1) {
      auto I1Size = llvm::LocationSize::beforeOrAfterPointer();
      llvm::Type *I1ElTy =
          llvm::cast<llvm::PointerType>((*I1)->getType())->getElementType();
      if (I1ElTy->isSized()) {
        I1Size = llvm::LocationSize::precise(DL.getTypeStoreSize(I1ElTy));
      }
      for (auto I2 = Pointers.begin(); I2 != I1; ++I2) {
        auto I2Size = llvm::LocationSize::beforeOrAfterPointer();
        llvm::Type *I2ElTy =
            llvm::cast<llvm::PointerType>((*I2)->getType())->getElementType();
        if (I2ElTy->isSized()) {
          I2Size = llvm::LocationSize::precise(DL.getTypeStoreSize(I2ElTy));
        }
        llvm::AliasResult AR = AA->alias(*I1, I1Size, *I2, I2Size);
        switch (AR) {
        case llvm::AliasResult::NoAlias:
          printResults(AR, PrintNoAlias, *I1, *I2, Fn->getParent());
          break;
        case llvm::AliasResult::MayAlias:
          printResults(AR, PrintMayAlias, *I1, *I2, Fn->getParent());
          break;
        case llvm::AliasResult::PartialAlias:
          printResults(AR, PrintPartialAlias, *I1, *I2, Fn->getParent());
          break;
        case llvm::AliasResult::MustAlias:
          printResults(AR, PrintMustAlias, *I1, *I2, Fn->getParent());
          break;
        }
      }
    }

    if (EvalAAMD) {
      // iterate over all pairs of load, store
      for (const llvm::Value *Load : Loads) {
        for (const llvm::Value *Store : Stores) {
          llvm::AliasResult AR = AA->alias(
              llvm::MemoryLocation::get(llvm::cast<llvm::LoadInst>(Load)),
              llvm::MemoryLocation::get(llvm::cast<llvm::StoreInst>(Store)));
          switch (AR) {
          case llvm::AliasResult::NoAlias:
            printLoadStoreResults(AR, PrintNoAlias, Load, Store,
                                  Fn->getParent());
            break;
          case llvm::AliasResult::MayAlias:
            printLoadStoreResults(AR, PrintMayAlias, Load, Store,
                                  Fn->getParent());
            break;
          case llvm::AliasResult::PartialAlias:
            printLoadStoreResults(AR, PrintPartialAlias, Load, Store,
                                  Fn->getParent());
            break;
          case llvm::AliasResult::MustAlias:
            printLoadStoreResults(AR, PrintMustAlias, Load, Store,
                                  Fn->getParent());
            break;
          }
        }
      }

      // iterate over all pairs of store, store
      for (auto I1 = Stores.begin(), E = Stores.end(); I1 != E; ++I1) {
        for (auto I2 = Stores.begin(); I2 != I1; ++I2) {
          llvm::AliasResult AR = AA->alias(
              llvm::MemoryLocation::get(llvm::cast<llvm::StoreInst>(*I1)),
              llvm::MemoryLocation::get(llvm::cast<llvm::StoreInst>(*I2)));
          switch (AR) {
          case llvm::AliasResult::NoAlias:
            printLoadStoreResults(AR, PrintNoAlias, *I1, *I2, Fn->getParent());
            break;
          case llvm::AliasResult::MayAlias:
            printLoadStoreResults(AR, PrintMayAlias, *I1, *I2, Fn->getParent());
            break;
          case llvm::AliasResult::PartialAlias:
            printLoadStoreResults(AR, PrintPartialAlias, *I1, *I2,
                                  Fn->getParent());
            break;
          case llvm::AliasResult::MustAlias:
            printLoadStoreResults(AR, PrintMustAlias, *I1, *I2,
                                  Fn->getParent());
            break;
          }
        }
      }
    }

    // Mod/ref alias analysis: compare all pairs of calls and values
    for (const llvm::CallBase *Call : Calls) {
      for (const auto *Pointer : Pointers) {
        auto Size = llvm::LocationSize::beforeOrAfterPointer();
        llvm::Type *ElTy =
            llvm::cast<llvm::PointerType>(Pointer->getType())->getElementType();
        if (ElTy->isSized()) {
          Size = llvm::LocationSize::precise(DL.getTypeStoreSize(ElTy));
        }

        switch (AA->getModRefInfo(Call, Pointer, Size)) {
        case llvm::ModRefInfo::NoModRef:
          printModRefResults("NoModRef", PrintNoModRef, Call, Pointer,
                             Fn->getParent());
          break;
        case llvm::ModRefInfo::Mod:
          printModRefResults("Just Mod", PrintMod, Call, Pointer,
                             Fn->getParent());
          break;
        case llvm::ModRefInfo::Ref:
          printModRefResults("Just Ref", PrintRef, Call, Pointer,
                             Fn->getParent());
          break;
        case llvm::ModRefInfo::ModRef:
          printModRefResults("Both ModRef", PrintModRef, Call, Pointer,
                             Fn->getParent());
          break;
        case llvm::ModRefInfo::Must:
          printModRefResults("Must", PrintMust, Call, Pointer, Fn->getParent());
          break;
        case llvm::ModRefInfo::MustMod:
          printModRefResults("Just Mod (MustAlias)", PrintMustMod, Call,
                             Pointer, Fn->getParent());
          break;
        case llvm::ModRefInfo::MustRef:
          printModRefResults("Just Ref (MustAlias)", PrintMustRef, Call,
                             Pointer, Fn->getParent());
          break;
        case llvm::ModRefInfo::MustModRef:
          printModRefResults("Both ModRef (MustAlias)", PrintMustModRef, Call,
                             Pointer, Fn->getParent());
          break;
        }
      }
    }

    // Mod/ref alias analysis: compare all pairs of calls
    for (const llvm::CallBase *CallA : Calls) {
      for (const llvm::CallBase *CallB : Calls) {
        if (CallA == CallB) {
          continue;
        }
        switch (AA->getModRefInfo(CallA, CallB)) {
        case llvm::ModRefInfo::NoModRef:
          printModRefResults("NoModRef", PrintNoModRef, CallA, CallB,
                             Fn->getParent());
          break;
        case llvm::ModRefInfo::Mod:
          printModRefResults("Just Mod", PrintMod, CallA, CallB,
                             Fn->getParent());
          break;
        case llvm::ModRefInfo::Ref:
          printModRefResults("Just Ref", PrintRef, CallA, CallB,
                             Fn->getParent());
          break;
        case llvm::ModRefInfo::ModRef:
          printModRefResults("Both ModRef", PrintModRef, CallA, CallB,
                             Fn->getParent());
          break;
        case llvm::ModRefInfo::Must:
          printModRefResults("Must", PrintMust, CallA, CallB, Fn->getParent());
          break;
        case llvm::ModRefInfo::MustMod:
          printModRefResults("Just Mod (MustAlias)", PrintMustMod, CallA, CallB,
                             Fn->getParent());
          break;
        case llvm::ModRefInfo::MustRef:
          printModRefResults("Just Ref (MustAlias)", PrintMustRef, CallA, CallB,
                             Fn->getParent());
          break;
        case llvm::ModRefInfo::MustModRef:
          printModRefResults("Both ModRef (MustAlias)", PrintMustModRef, CallA,
                             CallB, Fn->getParent());
          break;
        }
      }
    }
  }
}

} // namespace psr
