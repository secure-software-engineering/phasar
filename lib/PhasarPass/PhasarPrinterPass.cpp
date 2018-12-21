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
#include <phasar/PhasarPass/PhasarPrinterPass.h>

namespace psr {

char PhasarPrinterPass::ID = 12;

PhasarPrinterPass::PhasarPrinterPass() : llvm::ModulePass(ID) {}

llvm::StringRef PhasarPrinterPass::getPassName() const {
  return "PhasarPrinterPass";
}

bool PhasarPrinterPass::runOnModule(llvm::Module &M) {
  llvm::outs() << "PhasarPrinterPass::runOnModule()\n";
  PhasarPass &Results = getAnalysis<PhasarPass>();
  Results.print(llvm::outs(), &M);
  return false;
}

bool PhasarPrinterPass::doInitialization(llvm::Module &M) {
  llvm::outs() << "PhasarPrinterPass::doInitialization()\n";
  return false;
}

bool PhasarPrinterPass::doFinalization(llvm::Module &M) {
  llvm::outs() << "PhasarPrinterPass::doFinalization()\n";
  return false;
}

void PhasarPrinterPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<PhasarPass>();
  AU.setPreservesAll();
}

void PhasarPrinterPass::releaseMemory() {}

static llvm::RegisterPass<PhasarPrinterPass>
    phasar("phasar-printer", "PhASAR Printer Pass",
           false /* Only looks at CFG */, false /* Analysis Pass */);

} // namespace psr
