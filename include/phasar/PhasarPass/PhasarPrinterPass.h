/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARPASS_PHASARPRINTERPASS_H_
#define PHASAR_PHASARPASS_PHASARPRINTERPASS_H_

#include <llvm/Pass.h>

namespace llvm {
class Module;
class AnalysisUsage;
} // namespace llvm

namespace psr {

class PhasarPrinterPass : public llvm::ModulePass {
public:
  static char ID;

  explicit PhasarPrinterPass();
  PhasarPrinterPass(const PhasarPrinterPass &) = delete;
  PhasarPrinterPass &operator=(const PhasarPrinterPass &) = delete;
  ~PhasarPrinterPass() override = default;

  llvm::StringRef getPassName() const override;

  bool runOnModule(llvm::Module &M) override;

  bool doInitialization(llvm::Module &M) override;

  bool doFinalization(llvm::Module &M) override;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  void releaseMemory() override;
};

} // namespace psr

#endif
