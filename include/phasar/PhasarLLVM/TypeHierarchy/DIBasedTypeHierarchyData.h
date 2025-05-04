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

#include "llvm/ADT/StringMap.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <deque>
#include <string>

namespace psr {
struct DIBasedTypeHierarchyData {
  // DITypes and llvm::Function * are serialized by serializing their names and
  // using the DebugInfoFinder to deserialize them

  std::vector<std::string> VertexTypes;
  std::vector<std::pair<uint32_t, uint32_t>> TransitiveDerivedIndex;
  std::vector<std::string> Hierarchy;
  std::vector<std::vector<std::string>> VTables;

  DIBasedTypeHierarchyData() noexcept = default;
  void printAsJson(llvm::raw_ostream &OS);

  static DIBasedTypeHierarchyData deserializeJson(const llvm::Twine &Path);
  static DIBasedTypeHierarchyData loadJsonString(llvm::StringRef JsonAsString);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TYPEHIERARCHY_DIBASEDTYPEHIERARCHYDATA_H
