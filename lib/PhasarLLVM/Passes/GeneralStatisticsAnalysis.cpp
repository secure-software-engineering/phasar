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

#include <string>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_os_ostream.h"

#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysis.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"

using namespace std;
using namespace psr;

namespace psr {

llvm::AnalysisKey GeneralStatisticsAnalysis::Key; // NOLINT
GeneralStatistics
GeneralStatisticsAnalysis::run(llvm::Module &M,
                               llvm::ModuleAnalysisManager & /*AM*/) {
  PHASAR_LOG_LEVEL(INFO, "Running GeneralStatisticsAnalysis");
  static const std::set<std::string> MemAllocatingFunctions = {
      "operator new(unsigned long)", "operator new[](unsigned long)", "malloc",
      "calloc", "realloc"};
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
        } // check bitcast instructions for possible types
        else {
          for (auto *User : I.users()) {
            if (const llvm::BitCastInst *Cast =
                    llvm::dyn_cast<llvm::BitCastInst>(User)) {
              // types.insert(cast->getDestTy());
            }
          }
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
                            getNthFunctionArgument(CTor->getCalledFunction(), 0)
                                    ->getType() == Cast->getDestTy()) {
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
  for (auto &Global : M.globals()) {
    if (Global.getType()->isPointerTy()) {
      ++Stats.GlobalPointers;
    }
    ++Stats.Globals;
  }
  // register stuff in PAMM
  // For performance reasons (and out of sheer convenience) we simply initialize
  // the counter with the values of the counter varibles, i.e. PAMM simply
  // holds the results.
  PAMM_GET_INSTANCE;
  REG_COUNTER("GS Instructions", Stats.instructions, PAMM_SEVERITY_LEVEL::Core);
  REG_COUNTER("GS Allocated Types", Stats.allocatedTypes.size(),
              PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Allocation-Sites", Stats.allocationsites,
              PAMM_SEVERITY_LEVEL::Core);
  REG_COUNTER("GS Basic Blocks", Stats.basicblocks, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Call-Sites", Stats.callsites, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Functions", Stats.functions, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Globals", Stats.globals, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Global Pointer", Stats.globalPointers,
              PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Memory Intrinsics", Stats.memIntrinsic,
              PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Store Instructions", Stats.storeInstructions,
              PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Load Instructions", Stats.loadInstructions,
              PAMM_SEVERITY_LEVEL::Full);
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
      })
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

size_t GeneralStatistics::getMemoryIntrinsics() const { return MemIntrinsics; }

size_t GeneralStatistics::getStoreInstructions() const {
  return StoreInstructions;
}

set<const llvm::Type *> GeneralStatistics::getAllocatedTypes() const {
  return AllocatedTypes;
}

set<const llvm::Instruction *>
GeneralStatistics::getAllocaInstructions() const {
  return AllocaInstructions;
}

set<const llvm::Instruction *>
GeneralStatistics::getRetResInstructions() const {
  return RetResInstructions;
}

} // namespace psr
