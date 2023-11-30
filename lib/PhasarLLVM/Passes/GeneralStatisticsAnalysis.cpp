/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysis.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
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
#include "llvm/Support/raw_ostream.h"

#include <string>

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

llvm::AnalysisKey GeneralStatisticsAnalysis::Key; // NOLINT
GeneralStatistics GeneralStatisticsAnalysis::runOnModule(llvm::Module &M) {
  PHASAR_LOG_LEVEL(INFO, "Running GeneralStatisticsAnalysis");
  LLVMBasedCFG CFG;
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
            if (CFG.isHeapAllocatingFunction(CalleeFun)) {
              // do not add allocas from llvm internal functions
              Stats.AllocaInstructions.insert(&I);
              ++Stats.AllocationSites;
              // check if an instance of a user-defined type is allocated on the
              // heap
              for (auto *User : I.users()) {
                if (auto *Cast = llvm::dyn_cast<llvm::BitCastInst>(User)) {
                  if (Cast->getDestTy()
                          ->getPointerElementType()
                          ->isStructTy()) {
                    // finally check for ctor call
                    for (auto *User : Cast->users()) {
                      if (llvm::isa<llvm::CallInst>(User) ||
                          llvm::isa<llvm::InvokeInst>(User)) {
                        // potential call to the structures ctor
                        const llvm::CallBase *CTor =
                            llvm::cast<llvm::CallBase>(User);
                        if (CTor->getCalledFunction() &&
                            CTor->getCalledFunction()->getArg(0)->getType() ==
                                Cast->getDestTy()) {
                          Stats.AllocatedTypes.insert(
                              Cast->getDestTy()->getPointerElementType());
                        }
                      }
                    }
                  }
                }
              }
            }
          } else {
            ++Stats.IndCalls;
          }
        }
      }
    }
  }
  // check for global pointers
  for (auto const &Global : M.globals()) {
    ++Stats.Globals;
    if (Global.isConstant()) {
      ++Stats.GlobalConsts;
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
  IF_LOG_ENABLED(
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
      PHASAR_LOG_LEVEL(INFO,
                       "Store Instructions : " << Stats.StoreInstructions);
      PHASAR_LOG_LEVEL(INFO, ' '); PHASAR_LOG_LEVEL(
          INFO, "Allocated Types << " << Stats.AllocatedTypes.size());
      for (const auto *Type
           : Stats.AllocatedTypes) {
        PHASAR_LOG_LEVEL(INFO, "  " << llvmTypeToString(Type));
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
  OS << getAsJson() << '\n';
}

nlohmann::json GeneralStatistics::getAsJson() const {
  nlohmann::json J;
  J["ModuleName"] = ModuleName;
  J["Instructions"] = Instructions;
  J["Functions"] = Functions;
  J["ExternalFunctions"] = ExternalFunctions;
  J["FunctionDefinitions"] = FunctionDefinitions;
  J["AddressTakenFunctions"] = AddressTakenFunctions;
  J["AllocaInstructions"] = AllocaInstructions.size();
  J["CallSites"] = CallSites;
  J["IndirectCallSites"] = IndCalls;
  J["MemoryIntrinsics"] = MemIntrinsics;
  J["DebugIntrinsics"] = DebugIntrinsics;
  J["InlineAssembly"] = NumInlineAsm;
  J["GlobalVariables"] = Globals;
  J["Branches"] = Branches;
  J["GetElementPtrs"] = GetElementPtrs;
  J["BasicBlocks"] = BasicBlocks;
  J["PhiNodes"] = PhiNodes;
  J["LandingPads"] = LandingPads;
  J["GlobalConsts"] = GlobalConsts;
  return J;
}

} // namespace psr

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const GeneralStatistics &Statistics) {
  return OS << "General LLVM IR Statistics" << '\n'
            << "Module " << Statistics.ModuleName << ":\n"
            << "LLVM IR instructions:\t" << Statistics.Instructions << '\n'
            << "Functions:\t\t" << Statistics.Functions << '\n'
            << "External Functions:\t" << Statistics.ExternalFunctions << '\n'
            << "Function Definitions:\t" << Statistics.FunctionDefinitions
            << '\n'
            << "Address-Taken Functions:\t" << Statistics.AddressTakenFunctions
            << '\n'
            << "Globals:\t\t" << Statistics.Globals << '\n'
            << "Global Constants:\t" << Statistics.GlobalConsts << '\n'
            << "Global Variables:\t"
            << (Statistics.Globals - Statistics.GlobalConsts) << '\n'
            << "Alloca Instructions:\t" << Statistics.AllocaInstructions.size()
            << '\n'
            << "Call Sites:\t\t" << Statistics.CallSites << '\n'
            << "Indirect Call Sites:\t" << Statistics.IndCalls << '\n'
            << "Inline Assemblies:\t" << Statistics.NumInlineAsm << '\n'
            << "Memory Intrinsics:\t" << Statistics.MemIntrinsics << '\n'
            << "Debug Intrinsics:\t" << Statistics.DebugIntrinsics << '\n'
            << "Branches:\t" << Statistics.Branches << '\n'
            << "Switches:\t" << Statistics.Switches << '\n'
            << "GetElementPtrs:\t" << Statistics.GetElementPtrs << '\n'
            << "Phi Nodes:\t" << Statistics.PhiNodes << '\n'
            << "LandingPads:\t" << Statistics.LandingPads << '\n'
            << "Basic Blocks:\t" << Statistics.BasicBlocks << '\n'
            << "Avg #pred per BasicBlock:\t"
            << llvm::format("%g\n", double(Statistics.TotalNumPredecessorBBs) /
                                        double(Statistics.BasicBlocks))
            << "Max #pred per BasicBlock:\t" << Statistics.MaxNumPredecessorBBs
            << '\n'
            << "Avg #succ per BasicBlock:\t"
            << llvm::format("%g\n", double(Statistics.TotalNumSuccessorBBs) /
                                        double(Statistics.BasicBlocks))
            << "Max #succ per BasicBlock:\t" << Statistics.MaxNumSuccessorBBs
            << '\n'
            << "Avg #operands per Inst:\t\t"
            << llvm::format("%g\n", double(Statistics.TotalNumOperands) /
                                        double(Statistics.Instructions))
            << "Max #operands per Inst:\t\t" << Statistics.MaxNumOperands
            << '\n'
            << "Avg #uses per Inst:\t\t"
            << llvm::format("%g\n", double(Statistics.TotalNumUses) /
                                        double(Statistics.Instructions))
            << "Max #uses per Inst:\t\t" << Statistics.MaxNumUses << '\n'
            << "Insts with >1 uses:\t\t" << Statistics.NumInstWithMultipleUses
            << '\n'
            << "Non-void Insts:\t\t\t" << Statistics.NonVoidInsts << '\n'

      ;
}
