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

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/PassSupport.h>
#include <llvm/Support/raw_os_ostream.h>

#include <phasar/PhasarLLVM/Passes/GeneralStatisticsPass.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>
#include <phasar/Utils/PAMMMacros.h>

using namespace std;
using namespace psr;

namespace psr {

bool GeneralStatisticsPass::runOnModule(llvm::Module &M) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Running GeneralStatisticsPass");
  static const std::set<std::string> mem_allocating_functions = {
      "operator new(unsigned long)", "operator new[](unsigned long)", "malloc",
      "calloc", "realloc"};
  for (auto &F : M) {
    ++functions;
    for (auto &BB : F) {
      ++basicblocks;
      for (auto &I : BB) {
        // found one more instruction
        ++instructions;
        // check for alloca instruction for possible types
        if (const llvm::AllocaInst *alloc =
                llvm::dyn_cast<llvm::AllocaInst>(&I)) {
          allocatedTypes.insert(alloc->getAllocatedType());
          // do not add allocas from llvm internal functions
          allocaInstrucitons.insert(&I);
          ++allocationsites;
        } // check bitcast instructions for possible types
        else {
          for (auto user : I.users()) {
            if (const llvm::BitCastInst *cast =
                    llvm::dyn_cast<llvm::BitCastInst>(user)) {
              // types.insert(cast->getDestTy());
            }
          }
        }
        // check for return or resume instrucitons
        if (llvm::isa<llvm::ReturnInst>(I) || llvm::isa<llvm::ResumeInst>(I)) {
          retResInstructions.insert(&I);
        }
        // check for store instrucitons
        if (llvm::isa<llvm::StoreInst>(I)) {
          ++storeInstructions;
        }
        // check for llvm's memory intrinsics
        if (llvm::isa<llvm::MemIntrinsic>(I)) {
          ++memIntrinsic;
        }
        // check for function calls
        if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
          ++callsites;
          llvm::ImmutableCallSite CS(&I);
          if (CS.getCalledFunction()) {
            if (mem_allocating_functions.count(
                    cxx_demangle(CS.getCalledFunction()->getName().str()))) {
              // do not add allocas from llvm internal functions
              allocaInstrucitons.insert(&I);
              ++allocationsites;
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
                          allocatedTypes.insert(
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
      ++globalPointers;
    }
    ++globals;
  }
  return false;
}

bool GeneralStatisticsPass::doInitialization(llvm::Module &M) { return false; }

bool GeneralStatisticsPass::doFinalization(llvm::Module &M) {
  // For performance reasons (and out of sheer convenience) we simply initialize
  // the counter with the values of the counter varibles, i.e. PAMM simply
  // holds the results.
  PAMM_GET_INSTANCE;
  REG_COUNTER("GS Allocation-Sites", allocationsites,
              PAMM_SEVERITY_LEVEL::Core);
  REG_COUNTER("GS Instructions", instructions, PAMM_SEVERITY_LEVEL::Core);
  REG_COUNTER("GS Allocated Types", allocatedTypes.size(),
              PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Basic Blocks", basicblocks, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Call-Sites", callsites, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Functions", functions, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Globals", globals, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Global Pointer", globalPointers, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Memory Intrinsics", memIntrinsic, PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("GS Store Instructions", storeInstructions,
              PAMM_SEVERITY_LEVEL::Full);
  // Using the logging guard explicitly since we are printing allocated types
  // manually
  if (bl::core::get()->get_logging_enabled()) {
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, INFO) << "GeneralStatisticsPass summary for module: '"
                            << M.getName().str() << "'\n";
    BOOST_LOG_SEV(lg, INFO) << "Allocated Types    : " << allocatedTypes.size();
    BOOST_LOG_SEV(lg, INFO) << "Allocation Sites   : " << allocationsites;
    BOOST_LOG_SEV(lg, INFO) << "Basic Blocks       : " << basicblocks;
    BOOST_LOG_SEV(lg, INFO) << "Calls Sites        : " << callsites;
    BOOST_LOG_SEV(lg, INFO) << "Functions          : " << functions;
    BOOST_LOG_SEV(lg, INFO) << "Globals            : " << globals;
    BOOST_LOG_SEV(lg, INFO) << "Global Pointer     : " << globalPointers;
    BOOST_LOG_SEV(lg, INFO) << "Instructions       : " << instructions;
    BOOST_LOG_SEV(lg, INFO) << "Memory Intrinsics  : " << memIntrinsic;
    BOOST_LOG_SEV(lg, INFO) << "Store Instructions : " << storeInstructions;
    BOOST_LOG_SEV(lg, INFO) << ' ';
    for (auto type : allocatedTypes) {
      std::string type_str;
      llvm::raw_string_ostream rso(type_str);
      type->print(rso);
      BOOST_LOG_SEV(lg, INFO) << "  " << rso.str();
    }
  }
  return false;
}

void GeneralStatisticsPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

void GeneralStatisticsPass::releaseMemory() {}

size_t GeneralStatisticsPass::getAllocationsites() { return allocationsites; }

size_t GeneralStatisticsPass::getFunctioncalls() { return callsites; }

size_t GeneralStatisticsPass::getInstructions() { return instructions; }

size_t GeneralStatisticsPass::getGlobalPointers() { return globalPointers; }

set<const llvm::Type *> GeneralStatisticsPass::getAllocatedTypes() {
  return allocatedTypes;
}

set<const llvm::Value *> GeneralStatisticsPass::getAllocaInstructions() {
  return allocaInstrucitons;
}

set<const llvm::Instruction *> GeneralStatisticsPass::getRetResInstructions() {
  return retResInstructions;
}

} // namespace psr
