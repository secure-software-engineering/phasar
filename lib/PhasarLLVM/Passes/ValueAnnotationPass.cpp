/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ValueAnnotationPass.cpp
 *
 *  Created on: 26.01.2017
 *      Author: pdschbrt
 */

#include <iostream>
#include <string>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/PassSupport.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_os_ostream.h>

#include <phasar/Config/Configuration.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

namespace psr {

size_t ValueAnnotationPass::unique_value_id = 0;

bool ValueAnnotationPass::runOnModule(llvm::Module &M) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Running ValueAnnotationPass");
  for (auto &global : M.globals()) {
    llvm::MDNode *node = llvm::MDNode::get(
        context, llvm::MDString::get(context, std::to_string(unique_value_id)));
    global.setMetadata(MetaDataKind, node);
    //		std::cout <<
    // llvm::cast<llvm::MDString>(global.getMetadata(MetaDataKind)->getOperand(0))->getString().str()
    //<< std::endl;
    ++unique_value_id;
  }
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        llvm::MDNode *node = llvm::MDNode::get(
            context,
            llvm::MDString::get(context, std::to_string(unique_value_id)));
        I.setMetadata(MetaDataKind, node);
        //		    	std::cout <<
        // llvm::cast<llvm::MDString>(I.getMetadata(MetaDataKind)->getOperand(0))->getString().str()
        //<< std::endl;
        ++unique_value_id;
      }
    }
  }
  return true;
}

bool ValueAnnotationPass::doInitialization(llvm::Module &M) { return false; }

bool ValueAnnotationPass::doFinalization(llvm::Module &M) { return false; }

void ValueAnnotationPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesCFG();
}

void ValueAnnotationPass::releaseMemory() {}

void ValueAnnotationPass::resetValueID() {
  cout << "Reset ID" << endl;
  unique_value_id = 0;
}

} // namespace psr
