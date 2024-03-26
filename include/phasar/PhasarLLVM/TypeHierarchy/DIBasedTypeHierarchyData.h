/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_DIBASEDTYPEHIERARCHYDATA_H
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_DIBASEDTYPEHIERARCHYDATA_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/raw_ostream.h"

#include <deque>

namespace psr {
struct DIBasedTypeHierarchyData {
  DIBasedTypeHierarchyData() noexcept = default;

  // StringMap of ClassType as string
  llvm::StringMap<std::string> NameToType;
  // DenseMap of ClassType as string and according size_t
  llvm::DenseMap<std::string, size_t> TypeToVertex;
  // Vector of llvm::DICompositeType as string
  std::vector<std::string> VertexTypes;
  // Vector of pairs of uint32_ts
  std::vector<std::pair<uint32_t, uint32_t>> TransitiveDerivedIndex;
  // Vector of ClassType as string
  std::vector<std::string> Hierarchy;
  // Deque of LLVMVFTable (std::vector<const llvm::Function *>) as string
  std::deque<std::vector<std::string>> VTables;

  void printAsJson(llvm::raw_ostream &OS);
  static DIBasedTypeHierarchyData deserializeJson(const llvm::Twine &Path);
  static DIBasedTypeHierarchyData
  loadJsonString(const std::string &JsonAsString);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TYPEHIERARCHY_DIBASEDTYPEHIERARCHYDATA_H
