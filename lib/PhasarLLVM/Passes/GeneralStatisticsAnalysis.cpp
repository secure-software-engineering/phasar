/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysis.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"
#include "phasar/Utils/PAMMMacros.h"

#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <type_traits>

namespace psr {

static bool isAddressTaken(const llvm::Function &Fun) noexcept {
  for (const auto &Use : Fun.uses()) {
    const auto *Call = llvm::dyn_cast<llvm::CallBase>(Use.getUser());
    if (!Call || Use.get() != Call->getCalledOperand()) {
      return true;
    }
  }
  return false;
}

template <typename Set>
static void collectAllocatedTypes(const llvm::CallBase *CallSite, Set &Into) {
  for (const auto *User : CallSite->users()) {
    if (const auto *Cast = llvm::dyn_cast<llvm::BitCastInst>(User);
        Cast && Cast->getDestTy()->isPointerTy() &&
        !Cast->getDestTy()->isOpaquePointerTy()) {
      const auto *ElemTy = Cast->getDestTy()->getNonOpaquePointerElementType();
      if (ElemTy->isStructTy()) {
        // finally check for ctor call
        for (const auto *User : Cast->users()) {
          if (llvm::isa<llvm::CallBase>(User)) {
            // potential call to the structures ctor
            const auto *CTor = llvm::cast<llvm::CallBase>(User);
            if (CTor->getCalledFunction() &&
                CTor->getCalledFunction()->getArg(0)->getType() ==
                    Cast->getDestTy()) {
              Into.insert(ElemTy);
            }
          }
        }
      }
    }
  }
}

llvm::AnalysisKey GeneralStatisticsAnalysis::Key; // NOLINT
GeneralStatistics GeneralStatisticsAnalysis::runOnModule(llvm::Module &M) {
  PHASAR_LOG_LEVEL(INFO, "Running GeneralStatisticsAnalysis");
  Stats.ModuleName = M.getName().str();
  for (auto &F : M) {
    ++Stats.Functions;

    if (F.hasExternalLinkage()) {
      ++Stats.ExternalFunctions;
    }
    if (!F.isDeclaration()) {
      ++Stats.FunctionDefinitions;
    }

    if (isAddressTaken(F)) {
      ++Stats.AddressTakenFunctions;
    }

    for (auto &BB : F) {
      ++Stats.BasicBlocks;

      {
        auto PredSize = llvm::pred_size(&BB);
        auto SuccSize = llvm::succ_size(&BB);
        Stats.TotalNumPredecessorBBs += PredSize;
        Stats.TotalNumSuccessorBBs += SuccSize;
        if (PredSize > Stats.MaxNumPredecessorBBs) {
          ++Stats.MaxNumPredecessorBBs;
        }
        if (SuccSize > Stats.MaxNumSuccessorBBs) {
          ++Stats.MaxNumSuccessorBBs;
        }
      }

      for (auto &I : BB) {
        // found one more instruction
        ++Stats.Instructions;

        {
          auto NumOps = I.getNumOperands();
          auto NumUses = I.getNumUses();
          Stats.TotalNumOperands += NumOps;
          Stats.TotalNumUses += NumUses;
          if (NumOps > Stats.MaxNumOperands) {
            ++Stats.MaxNumOperands;
          }
          if (NumUses > Stats.MaxNumUses) {
            ++Stats.MaxNumUses;
          }
          if (NumUses > 1) {
            ++Stats.NumInstWithMultipleUses;
          }
        }

        if (!I.getType()->isVoidTy()) {
          ++Stats.NonVoidInsts;
        }

        if (I.isUsedOutsideOfBlock(I.getParent())) {
          ++Stats.NumInstsUsedOutsideBB;
        }

        // check for alloca instruction for possible types
        if (const llvm::AllocaInst *Alloc =
                llvm::dyn_cast<llvm::AllocaInst>(&I)) {
          Stats.AllocatedTypes.insert(Alloc->getAllocatedType());
          // do not add allocas from llvm internal functions
          Stats.AllocaInstructions.insert(&I);
          ++Stats.AllocationSites;
        }
        if (llvm::isa<llvm::PHINode>(I)) {
          ++Stats.PhiNodes;
        }
        if (llvm::isa<llvm::BranchInst>(I)) {
          ++Stats.Branches;
        }
        if (llvm::isa<llvm::SwitchInst>(I)) {
          ++Stats.Switches;
        }
        if (llvm::isa<llvm::GetElementPtrInst>(I)) {
          ++Stats.GetElementPtrs;
        }
        // check for return or resume instructions
        if (llvm::isa<llvm::ReturnInst>(I) || llvm::isa<llvm::ResumeInst>(I)) {
          Stats.RetResInstructions.insert(&I);
        }
        // check for store instructions
        if (llvm::isa<llvm::StoreInst>(I)) {
          ++Stats.StoreInstructions;
        }
        // check for load instructions
        if (llvm::isa<llvm::LoadInst>(I)) {
          ++Stats.LoadInstructions;
        }
        if (llvm::isa<llvm::LandingPadInst>(I)) {
          ++Stats.LandingPads;
        }
        // check for llvm's memory intrinsics
        if (llvm::isa<llvm::MemIntrinsic>(I)) {
          ++Stats.MemIntrinsics;
        }

        if (llvm::isa<llvm::DbgInfoIntrinsic>(I)) {
          ++Stats.DebugIntrinsics;
        }
        // check for function calls
        if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(&I)) {
          ++Stats.CallSites;

          const auto *CalledOp =
              CallSite->getCalledOperand()->stripPointerCastsAndAliases();

          if (llvm::isa<llvm::InlineAsm>(CalledOp)) {
            ++Stats.NumInlineAsm;
          } else if (const auto *CalleeFun =
                         llvm::dyn_cast<llvm::Function>(CalledOp)) {
            if (isHeapAllocatingFunction(CalleeFun)) {
              // do not add allocas from llvm internal functions
              Stats.AllocaInstructions.insert(&I);
              ++Stats.AllocationSites;
              // check if an instance of a user-defined type is allocated on the
              // heap
              collectAllocatedTypes(CallSite, Stats.AllocatedTypes);
            }
          } else {
            ++Stats.IndCalls;
          }
        }
      }
    }
  }
  // check for global pointers
  for (const auto &Global : M.globals()) {
    ++Stats.Globals;
    if (Global.isConstant()) {
      ++Stats.GlobalConsts;
    }
    if (!Global.isDeclaration()) {
      ++Stats.GlobalsDefinitions;
    }
    if (Global.hasExternalLinkage()) {
      ++Stats.ExternalGlobals;
    }
  }
  // register stuff in PAMM
  // For performance reasons (and out of sheer convenience) we simply initialize
  // the counter with the values of the counter varibles, i.e. PAMM simply
  // holds the results.
  PAMM_GET_INSTANCE;
  REG_COUNTER("GS Instructions", Stats.Instructions, Core);
  REG_COUNTER("GS Allocated Types", Stats.AllocatedTypes.size(), Full);
  REG_COUNTER("GS Basic Blocks", Stats.BasicBlocks, Full);
  REG_COUNTER("GS Call-Sites", Stats.CallSites, Full);
  REG_COUNTER("GS Functions", Stats.Functions, Full);
  REG_COUNTER("GS Globals", Stats.Globals, Full);
  REG_COUNTER("GS Memory Intrinsics", Stats.MemIntrinsics, Full);
  REG_COUNTER("GS Store Instructions", Stats.StoreInstructions, Full);
  REG_COUNTER("GS Load Instructions", Stats.LoadInstructions, Full);
  // Using the logging guard explicitly since we are printing allocated types
  // manually
  IF_LOG_LEVEL_ENABLED(INFO, {
    PHASAR_LOG_LEVEL(INFO, "GeneralStatisticsAnalysis summary for module: '"
                               << M.getName() << "'");
    PHASAR_LOG_LEVEL(INFO, "Instructions       : " << Stats.Instructions);
    PHASAR_LOG_LEVEL(INFO,
                     "Allocated Types    : " << Stats.AllocatedTypes.size());
    PHASAR_LOG_LEVEL(INFO, "Allocation Sites   : " << Stats.AllocationSites);
    PHASAR_LOG_LEVEL(INFO, "Basic Blocks       : " << Stats.BasicBlocks);
    PHASAR_LOG_LEVEL(INFO, "Calls Sites        : " << Stats.CallSites);
    PHASAR_LOG_LEVEL(INFO, "Functions          : " << Stats.Functions);
    PHASAR_LOG_LEVEL(INFO, "Globals            : " << Stats.Globals);
    PHASAR_LOG_LEVEL(INFO, "Global Consts      : " << Stats.GlobalConsts);
    PHASAR_LOG_LEVEL(INFO, "Memory Intrinsics  : " << Stats.MemIntrinsics);
    PHASAR_LOG_LEVEL(INFO, "Store Instructions : " << Stats.StoreInstructions);
    PHASAR_LOG_LEVEL(INFO, ' ');
    PHASAR_LOG_LEVEL(INFO,
                     "Allocated Types << " << Stats.AllocatedTypes.size());
    for (const auto *Type : Stats.AllocatedTypes) {
      PHASAR_LOG_LEVEL(INFO, "  " << llvmTypeToString(Type));
    }
  });
  // now we are done and can return the results
  return Stats;
}

size_t GeneralStatistics::getAllocationsites() const { return AllocationSites; }

size_t GeneralStatistics::getFunctioncalls() const { return CallSites; }

size_t GeneralStatistics::getInstructions() const { return Instructions; }

size_t GeneralStatistics::getGlobalPointers() const { return Globals; }

size_t GeneralStatistics::getBasicBlocks() const { return BasicBlocks; }

size_t GeneralStatistics::getFunctions() const { return Functions; }

size_t GeneralStatistics::getGlobals() const { return Globals; }

size_t GeneralStatistics::getGlobalConsts() const { return GlobalConsts; }

size_t GeneralStatistics::getMemoryIntrinsics() const { return MemIntrinsics; }

size_t GeneralStatistics::getStoreInstructions() const {
  return StoreInstructions;
}

const std::set<const llvm::Type *> &
GeneralStatistics::getAllocatedTypes() const {
  return AllocatedTypes;
}

const std::set<const llvm::Instruction *> &
GeneralStatistics::getAllocaInstructions() const {
  return AllocaInstructions;
}

const std::set<const llvm::Instruction *> &
GeneralStatistics::getRetResInstructions() const {
  return RetResInstructions;
}

void GeneralStatistics::printAsJson(llvm::raw_ostream &OS) const {
  nlohmann::json Json;

  Json["ModuleName"] = ModuleName;
  Json["Instructions"] = Instructions;
  Json["Functions"] = Functions;
  Json["ExternalFunctions"] = ExternalFunctions;
  Json["FunctionDefinitions"] = FunctionDefinitions;
  Json["AddressTakenFunctions"] = AddressTakenFunctions;
  Json["Globals"] = Globals;
  Json["GlobalConsts"] = GlobalConsts;
  Json["GlobalVariables"] = Globals - GlobalConsts;
  Json["ExternalGlobals"] = ExternalGlobals;
  Json["GlobalsDefinitions"] = GlobalsDefinitions;
  Json["AllocaInstructions"] = AllocaInstructions.size();
  Json["CallSites"] = CallSites;
  Json["IndirectCallSites"] = IndCalls;
  Json["NumInlineAssembly"] = NumInlineAsm;
  Json["MemoryIntrinsics"] = MemIntrinsics;
  Json["DebugIntrinsics"] = DebugIntrinsics;
  Json["Switches"] = Switches;
  Json["GetElementPtrs"] = GetElementPtrs;
  Json["PhiNodes"] = PhiNodes;
  Json["LandingPads"] = LandingPads;
  Json["BasicBlocks"] = BasicBlocks;
  Json["TotalNumPredecessorBBs"] = TotalNumPredecessorBBs;
  Json["Branches"] = Branches;
  Json["AvgPredPerBasicBlock"] =
      double(TotalNumPredecessorBBs) / double(BasicBlocks);
  Json["MaxPredPerBasicBlock"] = MaxNumPredecessorBBs;
  Json["AvgSuccPerBasicBlock"] =
      double(TotalNumSuccessorBBs) / double(BasicBlocks);
  Json["MaxSuccPerBasicBlock"] = MaxNumSuccessorBBs;
  Json["AvgOperandsPerInst"] = double(TotalNumOperands) / double(Instructions);
  Json["MaxNumOperandsPerInst"] = MaxNumOperands;
  Json["AvgUsesPerInst"] = double(TotalNumUses) / double(Instructions);
  Json["MaxUsesPerInst"] = MaxNumUses;
  Json["NumInstWithMultipleUses"] = NumInstWithMultipleUses;
  Json["NonVoidInsts"] = NonVoidInsts;
  Json["NumInstsUsedOutsideBB"] = NumInstsUsedOutsideBB;

  OS << Json << '\n';
}

nlohmann::json GeneralStatistics::getAsJson() const {
  std::string GeneralStatisticsAsString;
  llvm::raw_string_ostream Stream(GeneralStatisticsAsString);
  printAsJson(Stream);

  return nlohmann::json::parse(GeneralStatisticsAsString);
}

} // namespace psr

namespace {
template <typename T> struct AlignNum {
  llvm::StringRef Name;
  T Num;

  AlignNum(llvm::StringRef Name, T Num) noexcept : Name(Name), Num(Num) {}
  AlignNum(llvm::StringRef Name, size_t Numerator, size_t Denominator) noexcept
      : Name(Name), Num(double(Numerator) / double(Denominator)) {}

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const AlignNum &AN) {
    static constexpr size_t NumOffs = 32;

    auto Len = AN.Name.size() + 1;
    auto Diff = -(Len < NumOffs) & (NumOffs - Len);

    OS << AN.Name << ':';
    // Default is two fixed-point decimal places, so shift the output by three
    // spaces
    OS.indent(Diff + std::is_floating_point_v<T> * 3);
    OS << llvm::formatv("{0,+7}\n", AN.Num);

    return OS;
  }
};
template <typename T> AlignNum(llvm::StringRef, T) -> AlignNum<T>;
AlignNum(llvm::StringRef, size_t, size_t)->AlignNum<double>;
} // namespace

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const GeneralStatistics &Statistics) {
  return OS
         << "General LLVM IR Statistics\n"
         << "Module " << Statistics.ModuleName << ":\n"
         << "---------------------------------------\n"
         << AlignNum("LLVM IR instructions", Statistics.Instructions)
         << AlignNum("Functions", Statistics.Functions)
         << AlignNum("External Functions", Statistics.ExternalFunctions)
         << AlignNum("Function Definitions", Statistics.FunctionDefinitions)
         << AlignNum("Address-Taken Functions",
                     Statistics.AddressTakenFunctions)
         << AlignNum("Globals", Statistics.Globals)
         << AlignNum("Global Constants", Statistics.GlobalConsts)
         << AlignNum("Global Variables",
                     Statistics.Globals - Statistics.GlobalConsts)
         << AlignNum("External Globals", Statistics.ExternalGlobals)
         << AlignNum("Global Definitions", Statistics.GlobalsDefinitions)
         << AlignNum("Alloca Instructions",
                     Statistics.AllocaInstructions.size())
         << AlignNum("Call Sites", Statistics.CallSites)
         << AlignNum("Indirect Call Sites", Statistics.IndCalls)
         << AlignNum("Inline Assemblies", Statistics.NumInlineAsm)
         << AlignNum("Memory Intrinsics", Statistics.MemIntrinsics)
         << AlignNum("Debug Intrinsics", Statistics.DebugIntrinsics)
         << AlignNum("Switches", Statistics.Switches)
         << AlignNum("GetElementPtrs", Statistics.GetElementPtrs)
         << AlignNum("Phi Nodes", Statistics.PhiNodes)
         << AlignNum("LandingPads", Statistics.LandingPads)
         << AlignNum("Basic Blocks", Statistics.BasicBlocks)
         << AlignNum("Branches", Statistics.Branches)
         << AlignNum("Avg #pred per BasicBlock",
                     Statistics.TotalNumPredecessorBBs, Statistics.BasicBlocks)
         << AlignNum("Max #pred per BasicBlock",
                     Statistics.MaxNumPredecessorBBs)
         << AlignNum("Avg #succ per BasicBlock",
                     Statistics.TotalNumSuccessorBBs, Statistics.BasicBlocks)
         << AlignNum("Max #succ per BasicBlock", Statistics.MaxNumSuccessorBBs)
         << AlignNum("Avg #operands per Inst", Statistics.TotalNumOperands,
                     Statistics.Instructions)
         << AlignNum("Max #operands per Inst", Statistics.MaxNumOperands)
         << AlignNum("Avg #uses per Inst", Statistics.TotalNumUses,
                     Statistics.Instructions)
         << AlignNum("Max #uses per Inst", Statistics.MaxNumUses)
         << AlignNum("Insts with >1 uses", Statistics.NumInstWithMultipleUses)
         << AlignNum("Non-void Insts", Statistics.NonVoidInsts)
         << AlignNum("Insts used outside its BB",
                     Statistics.NumInstsUsedOutsideBB)

      ;
}
