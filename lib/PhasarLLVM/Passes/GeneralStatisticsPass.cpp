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

#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Support/raw_os_ostream.h>
#include <phasar/PhasarLLVM/Passes/GeneralStatisticsPass.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>
#include <phasar/Utils/PAMM.h>

using namespace std;

using namespace psr;
namespace psr {

bool GeneralStatisticsPass::runOnModule(llvm::Module &M) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, INFO) << "Running GeneralStatisticsPass";
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
      ++pointers;
    }
    ++globals;
  }
  return false;
}

bool GeneralStatisticsPass::doInitialization(llvm::Module &M) { return false; }

bool GeneralStatisticsPass::doFinalization(llvm::Module &M) {
#ifdef PERFORMANCE_EVA
  // add moduleID to counter names if performing MWA!
  // const std::string moduleID = " [" + M.getModuleIdentifier() + "]";
  // for performance reasons (and out of sheer convenience) we simply initialize
  // the counter with the values of the counter varibles, i.e. PAMM simply
  // holds the results.
  PAMM &pamm = PAMM::getInstance();
  pamm.regCounter("GS Functions" /* + moduleID*/, functions);
  pamm.regCounter("GS Globals" /* + moduleID*/, globals);
  pamm.regCounter("GS Basic Blocks" /* + moduleID*/, basicblocks);
  pamm.regCounter("GS Allocation-Sites" /* + moduleID*/, allocationsites);
  pamm.regCounter("GS Call-Sites" /* + moduleID*/, callsites);
  pamm.regCounter("GS Pointer Variables" /* + moduleID*/, pointers);
  pamm.regCounter("GS Instructions" /* + moduleID*/, instructions);
  pamm.regCounter("GS Store Instructions" /* + moduleID*/, storeInstructions);
  pamm.regCounter("GS Memory Intrinsics" /* + moduleID*/, memIntrinsic);
  pamm.regCounter("GS Allocated Types" /* + moduleID*/, allocatedTypes.size());
  return false;
#else
  llvm::outs() << "GeneralStatisticsPass summary for module: '"
               << M.getName().str() << "'\n";
  llvm::outs() << "functions: " << functions << "\n";
  llvm::outs() << "globals: " << globals << "\n";
  llvm::outs() << "basic blocks: " << basicblocks << "\n";
  llvm::outs() << "allocation sites: " << allocationsites << "\n";
  llvm::outs() << "calls-sites: " << callsites << "\n";
  llvm::outs() << "pointer variables: " << pointers << "\n";
  llvm::outs() << "instructions: " << instructions << "\n";
  llvm::outs() << "store instructions: " << storeInstructions << "\n";
  llvm::outs() << "llvm memory intrinsics: " << memIntrinsic << "\n";
  llvm::outs() << "allocated types:\n";
  for (auto type : allocatedTypes) {
    type->print(llvm::outs());
    llvm::outs() << " ";
  }
  llvm::outs() << "\n\n";
  return false;
#endif
}

void GeneralStatisticsPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

void GeneralStatisticsPass::releaseMemory() {}

size_t GeneralStatisticsPass::getAllocationsites() { return allocationsites; }

size_t GeneralStatisticsPass::getFunctioncalls() { return callsites; }

size_t GeneralStatisticsPass::getInstructions() { return instructions; }

size_t GeneralStatisticsPass::getPointers() { return pointers; }

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
