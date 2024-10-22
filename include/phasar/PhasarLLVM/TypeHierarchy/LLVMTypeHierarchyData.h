/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMTYPEHIERARCHYDATA_H_
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMTYPEHIERARCHYDATA_H_

#include "llvm/ADT/StringMap.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {
struct LLVMTypeHierarchyData {
  std::string PhasarConfigJsonTypeHierarchyID;
  // key = vertex, value = edges
  llvm::StringMap<std::vector<std::string>> TypeGraph;

  LLVMTypeHierarchyData() noexcept = default;
  void printAsJson(llvm::raw_ostream &OS);

  static LLVMTypeHierarchyData deserializeJson(const llvm::Twine &Path);
  static LLVMTypeHierarchyData loadJsonString(llvm::StringRef JsonAsString);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMTYPEHIERARCHYDATA_H_
