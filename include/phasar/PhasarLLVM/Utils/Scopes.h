/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_SCOPES_H_
#define PHASAR_PHASARLLVM_UTILS_SCOPES_H_

#include <map>
#include <string>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {

enum class Scope { function, module, project };

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, Scope S);

} // namespace psr

#endif
