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

#include <phasar/PhasarPass/Phasar.h>

namespace psr {

char Phasar::ID = 12;

Phasar::Phasar() : llvm::ModulePass(ID) {}

llvm::StringRef Phasar::getPassName() const { return "Phasar"; }

bool Phasar::runOnModule(llvm::Module &M) {
  llvm::outs() << "Phasar::runOnModule()\n";
  llvm::outs() << M << '\n';
  return false;
}

bool Phasar::doInitialization(llvm::Module &M) {
  llvm::outs() << "Phasar::doInitialization()\n";
  return false;
}

bool Phasar::doFinalization(llvm::Module &M) {
  llvm::outs() << "Phasar::doFinalization()\n";
  return false;
}

void Phasar::getAnalysisUsage(llvm::AnalysisUsage &AU) const {}

void Phasar::releaseMemory() {}

static llvm::RegisterPass<Phasar> phasar("phasar", "PhASAR Pass",
                                         false /* Only looks at CFG */,
                                         false /* Analysis Pass */);

}  // namespace psr
