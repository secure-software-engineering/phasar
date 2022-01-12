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
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_os_ostream.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

namespace psr {

llvm::AnalysisKey ValueAnnotationPass::Key;

size_t ValueAnnotationPass::UniqueValueId = 0;

ValueAnnotationPass::ValueAnnotationPass() = default;

llvm::PreservedAnalyses
ValueAnnotationPass::run(llvm::Module &M,
                         llvm::ModuleAnalysisManager & /*AM*/) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                << "Running ValueAnnotationPass");
  auto &Context = M.getContext();
  for (auto &Global : M.globals()) {
    llvm::MDNode *Node = llvm::MDNode::get(
        Context, llvm::MDString::get(Context, std::to_string(UniqueValueId)));
    Global.setMetadata(PhasarConfig::MetaDataKind(), Node);
    //		std::cout <<
    // llvm::cast<llvm::MDString>(global.getMetadata(MetaDataKind)->getOperand(0))->getString().str()
    //<< std::endl;
    ++UniqueValueId;
  }
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        llvm::MDNode *Node = llvm::MDNode::get(
            Context,
            llvm::MDString::get(Context, std::to_string(UniqueValueId)));
        I.setMetadata(PhasarConfig::MetaDataKind(), Node);
        //		    	std::cout <<
        // llvm::cast<llvm::MDString>(I.getMetadata(MetaDataKind)->getOperand(0))->getString().str()
        //<< std::endl;
        ++UniqueValueId;
      }
    }
  }
  return llvm::PreservedAnalyses::none();
}

void ValueAnnotationPass::resetValueID() {
  cout << "Reset ID" << endl;
  UniqueValueId = 0;
}

} // namespace psr
