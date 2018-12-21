/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Module.h>
#include <llvm/PassAnalysisSupport.h>
#include <llvm/Support/raw_ostream.h>

#include <phasar/PhasarPass/PhasarPass.h>

namespace psr {

char PhasarPass::ID = 12;

PhasarPass::PhasarPass() : llvm::ModulePass(ID) {}

llvm::StringRef PhasarPass::getPassName() const { return "PhasarPass"; }

bool PhasarPass::runOnModule(llvm::Module &M) {
  llvm::outs() << "PhasarPass::runOnModule()\n";
  llvm::outs() << M << '\n';
  return false;
}

bool PhasarPass::doInitialization(llvm::Module &M) {
  llvm::outs() << "PhasarPass::doInitialization()\n";
  return false;
}

bool PhasarPass::doFinalization(llvm::Module &M) {
  llvm::outs() << "PhasarPass::doFinalization()\n";
  return false;
}

void PhasarPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {}

void PhasarPass::releaseMemory() {}

void PhasarPass::print(llvm::raw_ostream &O, const llvm::Module *M) const {
  O << "I am a PhasarPass Analysis Result ;-)\n";
}

static llvm::RegisterPass<PhasarPass> phasar("phasar", "PhASAR Pass",
                                         false /* Only looks at CFG */,
                                         false /* Analysis Pass */);

}  // namespace psr
