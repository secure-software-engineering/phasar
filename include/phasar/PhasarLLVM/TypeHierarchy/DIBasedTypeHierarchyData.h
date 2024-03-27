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
  llvm::StringMap<unsigned int> NameToType;
  llvm::DenseMap<std::string, size_t> TypeToVertex;
  std::vector<unsigned int> VertexTypes;
  std::vector<std::pair<uint32_t, uint32_t>> TransitiveDerivedIndex;
  std::vector<std::string> Hierarchy;
  std::deque<std::vector<unsigned int>> VTables;

  DIBasedTypeHierarchyData() noexcept = default;
  void printAsJson(llvm::raw_ostream &OS);

  static DIBasedTypeHierarchyData deserializeJson(const llvm::Twine &Path);
  static DIBasedTypeHierarchyData loadJsonString(llvm::StringRef JsonAsString);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TYPEHIERARCHY_DIBASEDTYPEHIERARCHYDATA_H
