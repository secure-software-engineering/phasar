/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Utils/Scopes.h"

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS, Scope S) {
  switch (S) {
  case Scope::function:
    return OS << "function";
  case Scope::module:
    return OS << "module";
  case Scope::project:
    return OS << "project";
  }
  llvm_unreachable("All scopes should be handled in the switch above");
}
