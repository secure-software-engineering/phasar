/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARPASS_PHASARPASS_H_
#define PHASAR_PHASARPASS_PHASARPASS_H_

#include <llvm/Pass.h>

namespace llvm {
class Module;
class AnalysisUsage;
class raw_ostream;
} // namespace llvm

namespace psr {

class PhasarPass : public llvm::ModulePass {
public:
  static char ID;

  explicit PhasarPass();
  PhasarPass(const PhasarPass &) = delete;
  PhasarPass &operator=(const PhasarPass &) = delete;
  ~PhasarPass() override = default;

  llvm::StringRef getPassName() const override;

  bool runOnModule(llvm::Module &M) override;

  bool doInitialization(llvm::Module &M) override;

  bool doFinalization(llvm::Module &M) override;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  void releaseMemory() override;

  void print(llvm::raw_ostream &O, const llvm::Module *M) const override;
};

} // namespace psr

#endif
