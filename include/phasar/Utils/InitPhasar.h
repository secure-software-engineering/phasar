/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/Support/InitLLVM.h"

namespace psr {
class InitPhasar : llvm::InitLLVM {
public:
  InitPhasar(int &Argc, const char **&Argv) noexcept;
  InitPhasar(int &Argc, char **&Argv) noexcept
      : InitPhasar(Argc, (const char **&)Argv) {}
};
} // namespace psr

#define PSR_INITIALIZER(argc, argv)                                            \
  const ::psr::InitPhasar PsrInitializerVar((argc), (argv))
