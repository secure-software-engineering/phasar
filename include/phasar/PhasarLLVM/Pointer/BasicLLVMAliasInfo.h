/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_POINTER_BASICLLVMALIASINFO_H
#define PHASAR_POINTER_BASICLLVMALIASINFO_H

#include "phasar/Pointer/BasicAliasInfo.h"

#include "llvm/IR/Instruction.h"

namespace psr {

using BasicLLVMAliasInfoRef =
    BasicAliasInfoRef<const llvm::Value *, const llvm::Instruction *>;

static_assert(IsBasicAliasInfo<BasicLLVMAliasInfoRef>);

} // namespace psr

#endif // PHASAR_POINTER_BASICLLVMALIASINFO_H
