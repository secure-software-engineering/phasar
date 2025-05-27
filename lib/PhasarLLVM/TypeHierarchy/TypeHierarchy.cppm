module;

#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchyData.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchyData.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTableData.h"

export module phasar.llvm.typehierarchy;

export namespace psr {
using psr::DIBasedTypeHierarchy;
using psr::DIBasedTypeHierarchyData;
using psr::LLVMProjectIRDB;
using psr::LLVMTypeHierarchyData;
using psr::LLVMVFTable;
using psr::LLVMVFTableData;
} // namespace psr
