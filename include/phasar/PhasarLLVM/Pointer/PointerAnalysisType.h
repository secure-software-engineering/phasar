
/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_POINTERANALYSISTYPE_H_
#define PHASAR_PHASARLLVM_POINTER_POINTERANALYSISTYPE_H_

#include "llvm/ADT/StringRef.h"

#include <string>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {
enum class AliasResult { NoAlias, MayAlias, PartialAlias, MustAlias };

std::string toString(AliasResult AR);

AliasResult toAliasResult(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, AliasResult AR);

enum class PointerAnalysisType {
#define POINTER_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE) NAME,
#include "phasar/PhasarLLVM/Pointer/PointerAnalysisType.def"
  Invalid
};

std::string toString(PointerAnalysisType PA);

PointerAnalysisType toPointerAnalysisType(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, PointerAnalysisType PA);
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_POINTERANALYSISTYPE_H_
