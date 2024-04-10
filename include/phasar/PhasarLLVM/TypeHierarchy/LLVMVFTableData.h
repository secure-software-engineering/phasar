/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMVFTABLEDATA_H_
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMVFTABLEDATA_H_

#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {
struct LLVMVFTableData {
  std::vector<std::string> VFT;

  LLVMVFTableData() noexcept = default;
  void printAsJson(llvm::raw_ostream &OS) const;

  static LLVMVFTableData deserializeJson(const llvm::Twine &Path);
  static LLVMVFTableData loadJsonString(llvm::StringRef JsonAsString);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMVFTABLEDATA_H_
