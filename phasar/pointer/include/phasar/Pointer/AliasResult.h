/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_POINTER_ALIASRESULT_H
#define PHASAR_POINTER_ALIASRESULT_H

#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {
enum class AliasResult {
#define ALIAS_RESULT_TYPE(NAME) NAME,
#include "phasar/Pointer/AliasResult.def"
};

std::string toString(AliasResult AR);

AliasResult toAliasResult(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, AliasResult AR);
} // namespace psr

#endif // PHASAR_POINTER_ALIASRESULT_H
