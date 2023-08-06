/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MyHelloPass.cpp
 *
 *  Created on: 05.07.2016
 *      Author: pdschbrt
 */

#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysis.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

using namespace std;
using namespace psr;

namespace psr {

llvm::AnalysisKey GeneralStatisticsAnalysis::Key; // NOLINT
GeneralStatistics GeneralStatisticsAnalysis::runOnModule(llvm::Module &M) {
  PHASAR_LOG_LEVEL(INFO, "Running GeneralStatisticsAnalysis");
  static const std::set<std::string> MemAllocatingFunctions = {
      "operator new(unsigned long)", "operator new[](unsigned long)", "malloc",
      "calloc", "realloc"};
  Stats.ModuleName = M.getName();
  for (auto &F : M) {
    ++Stats.Functions;
    for (auto &BB : F) {
      ++Stats.BasicBlocks;
      for (auto &I : BB) {
        // found one more instruction
        ++Stats.Instructions;
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
        // check for llvm's memory intrinsics
        if (llvm::isa<llvm::MemIntrinsic>(I)) {
          ++Stats.MemIntrinsics;
        }
        // check for function calls
        if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
          ++Stats.CallSites;
          const llvm::CallBase *CallSite = llvm::cast<llvm::CallBase>(&I);
          if (CallSite->getCalledFunction()) {
            if (MemAllocatingFunctions.count(llvm::demangle(
                    CallSite->getCalledFunction()->getName().str()))) {
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
          }
        }
      }
    }
  }
  // check for global pointers
  for (auto const &Global : M.globals()) {
    if (Global.getType()->isPointerTy()) {
      ++Stats.GlobalPointers;
    }
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
      PHASAR_LOG_LEVEL(INFO, "Global Pointer     : " << Stats.GlobalPointers);
      PHASAR_LOG_LEVEL(INFO, "Global Consts      : " << Stats.GlobalConsts);
      PHASAR_LOG_LEVEL(INFO, "Memory Intrinsics  : " << Stats.MemIntrinsics);
      PHASAR_LOG_LEVEL(INFO,
                       "Store Instructions : " << Stats.StoreInstructions);
      PHASAR_LOG_LEVEL(INFO, ' '); PHASAR_LOG_LEVEL(
          INFO, "Allocated Types << " << Stats.AllocatedTypes.size());
      for (const auto *Type
           : Stats.AllocatedTypes) {
        std::string TypeStr;
        llvm::raw_string_ostream Rso(TypeStr);
        Type->print(Rso);
        PHASAR_LOG_LEVEL(INFO, "  " << Rso.str());
      });
  // now we are done and can return the results
  return Stats;
}

size_t GeneralStatistics::getAllocationsites() const { return AllocationSites; }

size_t GeneralStatistics::getFunctioncalls() const { return CallSites; }

size_t GeneralStatistics::getInstructions() const { return Instructions; }

size_t GeneralStatistics::getGlobalPointers() const { return GlobalPointers; }

size_t GeneralStatistics::getBasicBlocks() const { return BasicBlocks; }

size_t GeneralStatistics::getFunctions() const { return Functions; }

size_t GeneralStatistics::getGlobals() const { return Globals; }

size_t GeneralStatistics::getGlobalConsts() const { return GlobalConsts; }

size_t GeneralStatistics::getMemoryIntrinsics() const { return MemIntrinsics; }

size_t GeneralStatistics::getStoreInstructions() const {
  return StoreInstructions;
}

const set<const llvm::Type *> &GeneralStatistics::getAllocatedTypes() const {
  return AllocatedTypes;
}

const set<const llvm::Instruction *> &
GeneralStatistics::getAllocaInstructions() const {
  return AllocaInstructions;
}

const set<const llvm::Instruction *> &
GeneralStatistics::getRetResInstructions() const {
  return RetResInstructions;
}

void GeneralStatistics::printAsJson(llvm::raw_ostream &OS) const {
  OS << getAsJson().dump(4) << '\n';
}

nlohmann::json GeneralStatistics::getAsJson() const {
  nlohmann::json J;
  J["ModuleName"] = GeneralStatistics::ModuleName;
  J["Instructions"] = getInstructions();
  J["Functions"] = Functions;
  J["AllocaInstructions"] = AllocaInstructions.size();
  J["CallSites"] = CallSites;
  J["GlobalVariables"] = Globals;
  J["Branches"] = Branches;
  J["GetElementPtrs"] = GetElementPtrs;
  J["BasicBlocks"] = BasicBlocks;
  J["PhiNodes"] = PhiNodes;
  J["GlobalConsts"] = GlobalConsts;
  J["GlobalPointers"] = GlobalPointers;
  return J;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const GeneralStatistics &Statistics) {
  return OS << "General LLVM IR Statistics"
            << "\n"
            << "Module " << Statistics.ModuleName << ":\n"
            << "LLVM IR instructions:\t" << Statistics.Instructions << "\n"
            << "Functions:\t" << Statistics.Functions << "\n"
            << "Global Variables:\t" << Statistics.Globals << "\n"
            << "Global Variable Consts:\t" << Statistics.GlobalConsts << "\n"
            << "Global Pointers:\t" << Statistics.GlobalPointers << "\n"
            << "Alloca Instructions:\t" << Statistics.AllocaInstructions.size()
            << "\n"
            << "Call Sites:\t" << Statistics.CallSites << "\n"
            << "Branches:\t" << Statistics.Branches << "\n"
            << "GetElementPtrs:\t" << Statistics.GetElementPtrs << "\n"
            << "Phi Nodes:\t" << Statistics.PhiNodes << "\n"
            << "Basic Blocks:\t" << Statistics.BasicBlocks << "\n";
}

} // namespace psr
