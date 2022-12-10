/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/IR/Function.h"

bool psr::isHeapAllocatingFunction(const llvm::Function *Fun) {
  /// XXX: Consider using TargetLibraryInfo here
  return llvm::StringSwitch<bool>(Fun->getName())
      .Cases("malloc", "calloc", "realloc", "_Znwm", "_Znam", "_ZnwPv", true)
      .Default(false);
}
