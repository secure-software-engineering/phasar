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

  // llvm::StringMap<ClassType> NameToType
  // using ClassType = const llvm::DIType *
  llvm::StringMap<std::string> NameToType;
  // llvm::DenseMap<ClassType, size_t> TypeToVertex
  // using ClassType = const llvm::DIType *
  // Note: Using DenseMap<std::string, size_t> creates an error, therefore I've
  // resorted to using a StringMap
  llvm::StringMap<size_t> TypeToVertex;
  // std::vector<const llvm::DICompositeType *> VertexTypes
  std::vector<std::string> VertexTypes;
  // std::vector<std::pair<uint32_t, uint32_t>> TransitiveDerivedIndex
  std::vector<std::pair<uint32_t, uint32_t>> TransitiveDerivedIndex;
  // std::vector<ClassType> Hierarchy
  // using ClassType = const llvm::DIType *
  std::vector<std::string> Hierarchy;
  // std::deque<LLVMVFTable> VTables
  // Relevant data of LLVMVFTable:
  // std::vector<const llvm::Function *> VFT;
  std::deque<std::vector<std::string>> VTables;

  DIBasedTypeHierarchyData() noexcept = default;
  void printAsJson(llvm::raw_ostream &OS);

  static DIBasedTypeHierarchyData deserializeJson(const llvm::Twine &Path);
  static DIBasedTypeHierarchyData loadJsonString(llvm::StringRef JsonAsString);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TYPEHIERARCHY_DIBASEDTYPEHIERARCHYDATA_H
