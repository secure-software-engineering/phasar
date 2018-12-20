/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARPASS_PHASAR_H_
#define PHASAR_PHASARPASS_PHASAR_H_

#include <llvm/Pass.h>

namespace llvm {
class Module;
class AnalysisUsage;
}  // namespace llvm

namespace psr {

class Phasar : public llvm::ModulePass {
 public:
  static char ID;

  explicit Phasar();
  Phasar(const Phasar &) = delete;
  Phasar &operator=(const Phasar &) = delete;
  ~Phasar() override = default;

  llvm::StringRef getPassName() const override;

  bool runOnModule(llvm::Module &M) override;

  bool doInitialization(llvm::Module &M) override;

  bool doFinalization(llvm::Module &M) override;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  void releaseMemory() override;
};

}  // namespace psr

#endif
