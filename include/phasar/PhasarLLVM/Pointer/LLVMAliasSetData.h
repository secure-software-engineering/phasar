/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMALIASSETDATA_H
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMALIASSETDATA_H

#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {
struct LLVMAliasSetData {
  std::vector<std::vector<std::string>> AliasSets;
  std::vector<std::string> AnalyzedFunctions;

  LLVMAliasSetData() noexcept = default;
  void printAsJson(llvm::raw_ostream &OS);

  static LLVMAliasSetData deserializeJson(const llvm::Twine &Path);
  static LLVMAliasSetData loadJsonString(llvm::StringRef JsonAsString);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMALIASSETDATA_H
