/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMALIASINFO_H_
#define PHASAR_PHASARLLVM_POINTER_LLVMALIASINFO_H_

#include "phasar/Pointer/AliasInfo.h"

namespace llvm {
class Function;
class Instruction;
class Value;
} // namespace llvm

namespace psr {

using LLVMAliasInfoRef =
    AliasInfoRef<const llvm::Value *, const llvm::Instruction *>;

using LLVMAliasInfo = AliasInfo<const llvm::Value *, const llvm::Instruction *>;

} // namespace psr

#endif
