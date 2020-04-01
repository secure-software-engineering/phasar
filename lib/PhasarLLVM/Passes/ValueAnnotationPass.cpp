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

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/PassSupport.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_os_ostream.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

namespace psr {

llvm::AnalysisKey ValueAnnotationPass::Key;

size_t ValueAnnotationPass::unique_value_id = 0;

ValueAnnotationPass::ValueAnnotationPass() {}

llvm::PreservedAnalyses
ValueAnnotationPass::run(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Running ValueAnnotationPass");
  auto &context = M.getContext();
  for (auto &global : M.globals()) {
    llvm::MDNode *node = llvm::MDNode::get(
        context, llvm::MDString::get(context, std::to_string(unique_value_id)));
    global.setMetadata(PhasarConfig::MetaDataKind(), node);
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
        I.setMetadata(PhasarConfig::MetaDataKind(), node);
        //		    	std::cout <<
        // llvm::cast<llvm::MDString>(I.getMetadata(MetaDataKind)->getOperand(0))->getString().str()
        //<< std::endl;
        ++unique_value_id;
      }
    }
  }
  return llvm::PreservedAnalyses::none();
}

void ValueAnnotationPass::resetValueID() {
  cout << "Reset ID" << endl;
  unique_value_id = 0;
}

} // namespace psr
