/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARPASS_OPAQUEPTRTYPASS_H_
#define PHASAR_PHASARPASS_OPAQUEPTRTYPASS_H_

#include "llvm/Pass.h"

namespace llvm {
class Module;
} // namespace llvm

namespace psr {

class OpaquePtrTyPass : public llvm::ModulePass {
public:
  static inline char ID = 12; // NOLINT FIXME: make const when LLVM supports it

  explicit OpaquePtrTyPass();
  OpaquePtrTyPass(const OpaquePtrTyPass &) = delete;
  OpaquePtrTyPass &operator=(const OpaquePtrTyPass &) = delete;
  ~OpaquePtrTyPass() override = default;

  [[nodiscard]] llvm::StringRef getPassName() const override;

  bool runOnModule(llvm::Module &M) override;
};

} // namespace psr

#endif
