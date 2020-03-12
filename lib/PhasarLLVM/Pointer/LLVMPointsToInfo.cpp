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
#include "llvm/IR/Argument.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"

using namespace psr;

namespace psr {

static void PrintResults(llvm::AliasResult AR, bool P, const llvm::Value *V1,
                         const llvm::Value *V2, const llvm::Module *M) {
  if (P) {
    std::string o1, o2;
    {
      llvm::raw_string_ostream os1(o1), os2(o2);
      V1->printAsOperand(os1, true, M);
      V2->printAsOperand(os2, true, M);
    }

    if (o2 < o1) {
      std::swap(o1, o2);
    }
    llvm::errs() << "  " << AR << ":\t" << o1 << ", " << o2 << "\n";
  }
}

static inline void PrintModRefResults(const char *Msg, bool P,
                                      llvm::Instruction *I, llvm::Value *Ptr,
                                      llvm::Module *M) {
  if (P) {
    llvm::errs() << "  " << Msg << ":  Ptr: ";
    Ptr->printAsOperand(llvm::errs(), true, M);
    llvm::errs() << "\t<->" << *I << '\n';
  }
}

static inline void PrintModRefResults(const char *Msg, bool P,
                                      llvm::CallBase *CallA,
                                      llvm::CallBase *CallB, llvm::Module *M) {
  if (P) {
    llvm::errs() << "  " << Msg << ": " << *CallA << " <-> " << *CallB << '\n';
  }
}

static inline void PrintLoadStoreResults(llvm::AliasResult AR, bool P,
                                         const llvm::Value *V1,
                                         const llvm::Value *V2,
                                         const llvm::Module *M) {
  if (P) {
    llvm::errs() << "  " << AR << ": " << *V1 << " <-> " << *V2 << '\n';
  }
}

std::string to_string(const PointerAnalysisType &PA) {
  switch (PA) {
  default:
#define ANALYSIS_SETUP_POINTER_TYPE(NAME, CMDFLAG, TYPE)                       \
  case PointerAnalysisType::TYPE:                                              \
    return NAME;                                                               \
    break;
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
  }
}

PointerAnalysisType to_PointerAnalysisType(const std::string &S) {
  PointerAnalysisType Type = llvm::StringSwitch<PointerAnalysisType>(S)
#define ANALYSIS_SETUP_POINTER_TYPE(NAME, CMDFLAG, TYPE)                       \
  .Case(NAME, PointerAnalysisType::TYPE)
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
                                 .Default(PointerAnalysisType::Invalid);
  if (Type == PointerAnalysisType::Invalid) {
    Type = llvm::StringSwitch<PointerAnalysisType>(S)
#define ANALYSIS_SETUP_POINTER_TYPE(NAME, CMDFLAG, TYPE)                       \
  .Case(CMDFLAG, PointerAnalysisType::TYPE)
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
               .Default(PointerAnalysisType::Invalid);
  }
  return Type;
}

std::ostream &operator<<(std::ostream &os, const PointerAnalysisType &PA) {
  return os << to_string(PA);
}

LLVMPointsToInfo::LLVMPointsToInfo(ProjectIRDB &IRDB, PointerAnalysisType PAT) {
  // llvm::AAManager AA = PB.buildDefaultAAPipeline();
  llvm::AAManager AA;
  AA.registerFunctionAnalysis<llvm::BasicAA>();
  switch (PAT) {
  case PointerAnalysisType::CFLAnders:
    AA.registerFunctionAnalysis<llvm::CFLAndersAA>();
    break;
  case PointerAnalysisType::CFLSteens:
    AA.registerFunctionAnalysis<llvm::CFLSteensAA>();
    break;
  default:
    break;
  }
  FAM.registerPass([&] { return std::move(AA); });
  PB.registerFunctionAnalyses(FAM);
  llvm::FunctionPassManager FPM;
  // Always verify the input.
  FPM.addPass(llvm::VerifierPass());
  for (llvm::Module *M : IRDB.getAllModules()) {
    for (auto &F : *M) {
      if (!F.isDeclaration()) {
        llvm::PreservedAnalyses PA = FPM.run(F, FAM);
        llvm::AAResults &AAR = FAM.getResult<llvm::AAManager>(F);
        AAInfos.insert(std::make_pair(&F, &AAR));
        PointsToGraphs.insert(std::make_pair(
            &F, std::make_unique<PointsToGraph>(&F, *AAInfos.at(&F))));
      }
    }
  }
}

AliasResult LLVMPointsToInfo::alias(const llvm::Value *V1,
                                    const llvm::Value *V2,
                                    const llvm::Instruction *I) {
  // delegate to LLVM's intra-procedural alias analysis results
  const llvm::Function *V1F;
  const llvm::Function *V2F;
  // determine V1F
  if (auto T = llvm::dyn_cast<llvm::Instruction>(V1)) {
    V1F = T->getFunction();
  }
  if (auto T = llvm::dyn_cast<llvm::BasicBlock>(V1)) {
    V1F = T->getParent();
  }
  if (auto T = llvm::dyn_cast<llvm::Argument>(V1)) {
    V1F = T->getParent();
  }
  // determine V2F
  if (auto T = llvm::dyn_cast<llvm::Instruction>(V2)) {
    V2F = T->getFunction();
  }
  if (auto T = llvm::dyn_cast<llvm::BasicBlock>(V2)) {
    V2F = T->getParent();
  }
  if (auto T = llvm::dyn_cast<llvm::Argument>(V2)) {
    V2F = T->getParent();
  }
  if (V1F != V2F) {
    // don't know, used intra-procedural information for inter-procedural query
    return AliasResult::MayAlias;
  }
  switch (AAInfos.at(V1F)->alias(V1, V2)) {
  case llvm::NoAlias:
    return AliasResult::NoAlias;
    break;
  case llvm::MayAlias:
    return AliasResult::MayAlias;
    break;
  case llvm::PartialAlias:
    return AliasResult::PartialAlias;
    break;
  case llvm::MustAlias:
    return AliasResult::MustAlias;
    break;
  }
}

std::set<const llvm::Value *>
LLVMPointsToInfo::getPointsToSet(const llvm::Value *V,
                                 const llvm::Instruction *I) const {
  // Value can be one of
  //  -Argument
  //  -BasicBlock
  //  -InlineAsm
  //  -MetadataAsValue
  //  -User
  if (auto Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return getPointsToGraph(Arg->getParent())->getPointsToSet(V);
  }
  if (auto Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
    return getPointsToGraph(Inst->getFunction())->getPointsToSet(V);
  }
  return {};
}

void LLVMPointsToInfo::print(std::ostream &OS) const {
  OS << "Points-to Info:\n";
  for (auto &[Fn, AA] : AAInfos) {
    bool PrintAll, PrintNoAlias, PrintMayAlias, PrintPartialAlias,
        PrintMustAlias, EvalAAMD, PrintNoModRef, PrintMod, PrintRef,
        PrintModRef, PrintMust, PrintMustMod, PrintMustRef, PrintMustModRef;
    PrintAll = PrintNoAlias = PrintMayAlias = PrintPartialAlias =
        PrintMustAlias = EvalAAMD = PrintNoModRef = PrintMod = PrintRef =
            PrintModRef = PrintMust = PrintMustMod = PrintMustRef =
                PrintMustModRef = true;

    // taken from llvm/Analysis/AliasAnalysisEvaluator.cpp
    const llvm::DataLayout &DL = Fn->getParent()->getDataLayout();

    llvm::Function *F = const_cast<llvm::Function *>(Fn);

    llvm::SetVector<llvm::Value *> Pointers;
    llvm::SmallSetVector<llvm::CallBase *, 16> Calls;
    llvm::SetVector<llvm::Value *> Loads;
    llvm::SetVector<llvm::Value *> Stores;

    for (auto &I : F->args()) {
      if (I.getType()->isPointerTy()) { // Add all pointer arguments.
        Pointers.insert(&I);
      }
    }

    for (llvm::inst_iterator I = inst_begin(*F), E = inst_end(*F); I != E;
         ++I) {
      if (I->getType()->isPointerTy()) // Add all pointer instructions.
        Pointers.insert(&*I);
      if (EvalAAMD && llvm::isa<llvm::LoadInst>(&*I))
        Loads.insert(&*I);
      if (EvalAAMD && llvm::isa<llvm::StoreInst>(&*I))
        Stores.insert(&*I);
      llvm::Instruction &Inst = *I;
      if (auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst)) {
        llvm::Value *Callee = Call->getCalledValue();
        // Skip actual functions for direct function calls.
        if (!llvm::isa<llvm::Function>(Callee) && isInterestingPointer(Callee))
          Pointers.insert(Callee);
        // Consider formals.
        for (llvm::Use &DataOp : Call->data_ops())
          if (isInterestingPointer(DataOp))
            Pointers.insert(DataOp);
        Calls.insert(Call);
      } else {
        // Consider all operands.
        for (llvm::Instruction::op_iterator OI = Inst.op_begin(),
                                            OE = Inst.op_end();
             OI != OE; ++OI)
          if (isInterestingPointer(*OI))
            Pointers.insert(*OI);
      }
    }

    if (PrintAll || PrintNoAlias || PrintMayAlias || PrintPartialAlias ||
        PrintMustAlias || PrintNoModRef || PrintMod || PrintRef || PrintModRef)
      OS << "Function: " << F->getName().str() << ": " << Pointers.size()
         << " pointers, " << Calls.size() << " call sites\n";

    // iterate over the worklist, and run the full (n^2)/2 disambiguations
    for (llvm::SetVector<llvm::Value *>::iterator I1 = Pointers.begin(),
                                                  E = Pointers.end();
         I1 != E; ++I1) {
      auto I1Size = llvm::LocationSize::unknown();
      llvm::Type *I1ElTy =
          llvm::cast<llvm::PointerType>((*I1)->getType())->getElementType();
      if (I1ElTy->isSized()) {
        I1Size = llvm::LocationSize::precise(DL.getTypeStoreSize(I1ElTy));
      }
      for (llvm::SetVector<llvm::Value *>::iterator I2 = Pointers.begin();
           I2 != I1; ++I2) {
        auto I2Size = llvm::LocationSize::unknown();
        llvm::Type *I2ElTy =
            llvm::cast<llvm::PointerType>((*I2)->getType())->getElementType();
        if (I2ElTy->isSized()) {
          I2Size = llvm::LocationSize::precise(DL.getTypeStoreSize(I2ElTy));
        }
        llvm::AliasResult AR = AA->alias(*I1, I1Size, *I2, I2Size);
        switch (AR) {
        case llvm::NoAlias:
          PrintResults(AR, PrintNoAlias, *I1, *I2, F->getParent());
          break;
        case llvm::MayAlias:
          PrintResults(AR, PrintMayAlias, *I1, *I2, F->getParent());
          break;
        case llvm::PartialAlias:
          PrintResults(AR, PrintPartialAlias, *I1, *I2, F->getParent());
          break;
        case llvm::MustAlias:
          PrintResults(AR, PrintMustAlias, *I1, *I2, F->getParent());
          break;
        }
      }
    }

    if (EvalAAMD) {
      // iterate over all pairs of load, store
      for (llvm::Value *Load : Loads) {
        for (llvm::Value *Store : Stores) {
          llvm::AliasResult AR = AA->alias(
              llvm::MemoryLocation::get(llvm::cast<llvm::LoadInst>(Load)),
              llvm::MemoryLocation::get(llvm::cast<llvm::StoreInst>(Store)));
          switch (AR) {
          case llvm::NoAlias:
            PrintLoadStoreResults(AR, PrintNoAlias, Load, Store,
                                  F->getParent());
            break;
          case llvm::MayAlias:
            PrintLoadStoreResults(AR, PrintMayAlias, Load, Store,
                                  F->getParent());
            break;
          case llvm::PartialAlias:
            PrintLoadStoreResults(AR, PrintPartialAlias, Load, Store,
                                  F->getParent());
            break;
          case llvm::MustAlias:
            PrintLoadStoreResults(AR, PrintMustAlias, Load, Store,
                                  F->getParent());
            break;
          }
        }
      }

      // iterate over all pairs of store, store
      for (llvm::SetVector<llvm::Value *>::iterator I1 = Stores.begin(),
                                                    E = Stores.end();
           I1 != E; ++I1) {
        for (llvm::SetVector<llvm::Value *>::iterator I2 = Stores.begin();
             I2 != I1; ++I2) {
          llvm::AliasResult AR = AA->alias(
              llvm::MemoryLocation::get(llvm::cast<llvm::StoreInst>(*I1)),
              llvm::MemoryLocation::get(llvm::cast<llvm::StoreInst>(*I2)));
          switch (AR) {
          case llvm::NoAlias:
            PrintLoadStoreResults(AR, PrintNoAlias, *I1, *I2, F->getParent());
            break;
          case llvm::MayAlias:
            PrintLoadStoreResults(AR, PrintMayAlias, *I1, *I2, F->getParent());
            break;
          case llvm::PartialAlias:
            PrintLoadStoreResults(AR, PrintPartialAlias, *I1, *I2,
                                  F->getParent());
            break;
          case llvm::MustAlias:
            PrintLoadStoreResults(AR, PrintMustAlias, *I1, *I2, F->getParent());
            break;
          }
        }
      }
    }

    // Mod/ref alias analysis: compare all pairs of calls and values
    for (llvm::CallBase *Call : Calls) {
      for (auto Pointer : Pointers) {
        auto Size = llvm::LocationSize::unknown();
        llvm::Type *ElTy =
            llvm::cast<llvm::PointerType>(Pointer->getType())->getElementType();
        if (ElTy->isSized())
          Size = llvm::LocationSize::precise(DL.getTypeStoreSize(ElTy));

        switch (AA->getModRefInfo(Call, Pointer, Size)) {
        case llvm::ModRefInfo::NoModRef:
          PrintModRefResults("NoModRef", PrintNoModRef, Call, Pointer,
                             F->getParent());
          break;
        case llvm::ModRefInfo::Mod:
          PrintModRefResults("Just Mod", PrintMod, Call, Pointer,
                             F->getParent());
          break;
        case llvm::ModRefInfo::Ref:
          PrintModRefResults("Just Ref", PrintRef, Call, Pointer,
                             F->getParent());
          break;
        case llvm::ModRefInfo::ModRef:
          PrintModRefResults("Both ModRef", PrintModRef, Call, Pointer,
                             F->getParent());
          break;
        case llvm::ModRefInfo::Must:
          PrintModRefResults("Must", PrintMust, Call, Pointer, F->getParent());
          break;
        case llvm::ModRefInfo::MustMod:
          PrintModRefResults("Just Mod (MustAlias)", PrintMustMod, Call,
                             Pointer, F->getParent());
          break;
        case llvm::ModRefInfo::MustRef:
          PrintModRefResults("Just Ref (MustAlias)", PrintMustRef, Call,
                             Pointer, F->getParent());
          break;
        case llvm::ModRefInfo::MustModRef:
          PrintModRefResults("Both ModRef (MustAlias)", PrintMustModRef, Call,
                             Pointer, F->getParent());
          break;
        }
      }
    }

    // Mod/ref alias analysis: compare all pairs of calls
    for (llvm::CallBase *CallA : Calls) {
      for (llvm::CallBase *CallB : Calls) {
        if (CallA == CallB)
          continue;
        switch (AA->getModRefInfo(CallA, CallB)) {
        case llvm::ModRefInfo::NoModRef:
          PrintModRefResults("NoModRef", PrintNoModRef, CallA, CallB,
                             F->getParent());
          break;
        case llvm::ModRefInfo::Mod:
          PrintModRefResults("Just Mod", PrintMod, CallA, CallB,
                             F->getParent());
          break;
        case llvm::ModRefInfo::Ref:
          PrintModRefResults("Just Ref", PrintRef, CallA, CallB,
                             F->getParent());
          break;
        case llvm::ModRefInfo::ModRef:
          PrintModRefResults("Both ModRef", PrintModRef, CallA, CallB,
                             F->getParent());
          break;
        case llvm::ModRefInfo::Must:
          PrintModRefResults("Must", PrintMust, CallA, CallB, F->getParent());
          break;
        case llvm::ModRefInfo::MustMod:
          PrintModRefResults("Just Mod (MustAlias)", PrintMustMod, CallA, CallB,
                             F->getParent());
          break;
        case llvm::ModRefInfo::MustRef:
          PrintModRefResults("Just Ref (MustAlias)", PrintMustRef, CallA, CallB,
                             F->getParent());
          break;
        case llvm::ModRefInfo::MustModRef:
          PrintModRefResults("Both ModRef (MustAlias)", PrintMustModRef, CallA,
                             CallB, F->getParent());
          break;
        }
      }
    }
  }
}

nlohmann::json LLVMPointsToInfo::getAsJson() const { return ""_json; }

PointsToGraph *
LLVMPointsToInfo::getPointsToGraph(const llvm::Function *F) const {
  if (PointsToGraphs.count(F)) {
    return PointsToGraphs.at(F).get();
  }
  return nullptr;
}

llvm::AAResults *LLVMPointsToInfo::getAAResults(const llvm::Function *F) const {
  if (AAInfos.count(F)) {
    return AAInfos.at(F);
  }
  return nullptr;
}

} // namespace psr
