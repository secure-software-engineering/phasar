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
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/PassSupport.h"
#include "llvm/Support/raw_os_ostream.h"

#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysis.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"

using namespace std;
using namespace psr;

namespace psr {

llvm::AnalysisKey GeneralStatisticsAnalysis::Key;

GeneralStatisticsAnalysis::GeneralStatisticsAnalysis() {}

GeneralStatistics
GeneralStatisticsAnalysis::run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &AM) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Running GeneralStatisticsAnalysis");
  static const std::set<std::string> mem_allocating_functions = {
      "operator new(unsigned long)", "operator new[](unsigned long)", "malloc",
      "calloc", "realloc"};
  for (auto &F : M) {
    ++Stats.functions;
    for (auto &BB : F) {
      ++Stats.basicblocks;
      for (auto &I : BB) {
        // found one more instruction
        ++Stats.instructions;
        // check for alloca instruction for possible types
        if (const llvm::AllocaInst *alloc =
                llvm::dyn_cast<llvm::AllocaInst>(&I)) {
          Stats.allocatedTypes.insert(alloc->getAllocatedType());
          // do not add allocas from llvm internal functions
          Stats.allocaInstructions.insert(&I);
          ++Stats.allocationsites;
        } // check bitcast instructions for possible types
        else {
          for (auto user : I.users()) {
            if (const llvm::BitCastInst *cast =
                    llvm::dyn_cast<llvm::BitCastInst>(user)) {
              // types.insert(cast->getDestTy());
            }
          }
        }
        // check for return or resume instructions
        if (llvm::isa<llvm::ReturnInst>(I) || llvm::isa<llvm::ResumeInst>(I)) {
          Stats.retResInstructions.insert(&I);
        }
        // check for store instructions
        if (llvm::isa<llvm::StoreInst>(I)) {
          ++Stats.storeInstructions;
        }
        // check for load instructions
        if (llvm::isa<llvm::LoadInst>(I)) {
          ++Stats.loadInstructions;
        }
        // check for llvm's memory intrinsics
        if (llvm::isa<llvm::MemIntrinsic>(I)) {
          ++Stats.memIntrinsic;
        }
        // check for function calls
        if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
          ++Stats.callsites;
          llvm::ImmutableCallSite CS(&I);
          if (CS.getCalledFunction()) {
            if (mem_allocating_functions.count(
                    cxx_demangle(CS.getCalledFunction()->getName().str()))) {
              // do not add allocas from llvm internal functions
              Stats.allocaInstructions.insert(&I);
              ++Stats.allocationsites;
              // check if an instance of a user-defined type is allocated on the
              // heap
              for (auto User : I.users()) {
                if (auto Cast = llvm::dyn_cast<llvm::BitCastInst>(User)) {
                  if (Cast->getDestTy()
                          ->getPointerElementType()
                          ->isStructTy()) {
                    // finally check for ctor call
                    for (auto User : Cast->users()) {
                      if (llvm::isa<llvm::CallInst>(User) ||
                          llvm::isa<llvm::InvokeInst>(User)) {
                        // potential call to the structures ctor
                        llvm::ImmutableCallSite CTor(User);
                        if (CTor.getCalledFunction() &&
                            getNthFunctionArgument(CTor.getCalledFunction(), 0)
                                    ->getType() == Cast->getDestTy()) {
                          Stats.allocatedTypes.insert(
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
  for (auto &global : M.globals()) {
    if (global.getType()->isPointerTy()) {
      ++Stats.globalPointers;
    }
    ++Stats.globals;
  }
  // register stuff in PAMM
  // For performance reasons (and out of sheer convenience) we simply initialize
  // the counter with the values of the counter varibles, i.e. PAMM simply
  // holds the results.
  PAMM_GET_INSTANCE;
  REG_COUNTER("GS Instructions", instructions, PAMM_SEVERITY_LEVEL::Core);
  REG_COUNTER("GS Allocated Types", allocatedTypes.size(),
              PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Allocation-Sites", allocationsites,
              PAMM_SEVERITY_LEVEL::Core);
  REG_COUNTER("GS Basic Blocks", basicblocks, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Call-Sites", callsites, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Functions", functions, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Globals", globals, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Global Pointer", globalPointers, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Memory Intrinsics", memIntrinsic, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Store Instructions", storeInstructions,
              PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Load Instructions", loadInstructions,
              PAMM_SEVERITY_LEVEL::Full);
  // Using the logging guard explicitly since we are printing allocated types
  // manually
  if (boost::log::core::get()->get_logging_enabled()) {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, INFO) << "GeneralStatisticsAnalysis summary for module: '"
                            << M.getName().str() << "'";
    BOOST_LOG_SEV(lg, INFO) << "Instructions       : " << Stats.instructions;
    BOOST_LOG_SEV(lg, INFO)
        << "Allocated Types    : " << Stats.allocatedTypes.size();
    BOOST_LOG_SEV(lg, INFO) << "Allocation Sites   : " << Stats.allocationsites;
    BOOST_LOG_SEV(lg, INFO) << "Basic Blocks       : " << Stats.basicblocks;
    BOOST_LOG_SEV(lg, INFO) << "Calls Sites        : " << Stats.callsites;
    BOOST_LOG_SEV(lg, INFO) << "Functions          : " << Stats.functions;
    BOOST_LOG_SEV(lg, INFO) << "Globals            : " << Stats.globals;
    BOOST_LOG_SEV(lg, INFO) << "Global Pointer     : " << Stats.globalPointers;
    BOOST_LOG_SEV(lg, INFO) << "Memory Intrinsics  : " << Stats.memIntrinsic;
    BOOST_LOG_SEV(lg, INFO)
        << "Store Instructions : " << Stats.storeInstructions;
    BOOST_LOG_SEV(lg, INFO) << ' ';
    for (auto type : Stats.allocatedTypes) {
      std::string type_str;
      llvm::raw_string_ostream rso(type_str);
      type->print(rso);
      BOOST_LOG_SEV(lg, INFO) << "  " << rso.str();
    }
  }
  // now we are done and can return the results
  return Stats;
}

size_t GeneralStatistics::getAllocationsites() const { return allocationsites; }

size_t GeneralStatistics::getFunctioncalls() const { return callsites; }

size_t GeneralStatistics::getInstructions() const { return instructions; }

size_t GeneralStatistics::getGlobalPointers() const { return globalPointers; }

size_t GeneralStatistics::getBasicBlocks() const { return basicblocks; }

size_t GeneralStatistics::getFunctions() const { return functions; }

size_t GeneralStatistics::getGlobals() const { return globals; }

size_t GeneralStatistics::getMemoryIntrinsics() const { return memIntrinsic; }

size_t GeneralStatistics::getStoreInstructions() const {
  return storeInstructions;
}

set<const llvm::Type *> GeneralStatistics::getAllocatedTypes() const {
  return allocatedTypes;
}

set<const llvm::Instruction *>
GeneralStatistics::getAllocaInstructions() const {
  return allocaInstructions;
}

set<const llvm::Instruction *>
GeneralStatistics::getRetResInstructions() const {
  return retResInstructions;
}

} // namespace psr
