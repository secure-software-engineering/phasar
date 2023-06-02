#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy_MD.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"

#include <llvm-14/llvm/IR/DerivedTypes.h>
#include <llvm-14/llvm/IR/GlobalVariable.h>
#include <llvm-14/llvm/IR/Metadata.h>

namespace psr {

LLVMTypeHierarchy_MD::VertexProperties::VertexProperties(
    const llvm::StructType *Type)
    : Type(Type), ReachableTypes({Type}) {}

std::string LLVMTypeHierarchy_MD::VertexProperties::getTypeName() const {
  return Type->getStructName().str();
}

LLVMTypeHierarchy_MD::LLVMTypeHierarchy_MD(LLVMProjectIRDB &IRDB) {
  PHASAR_LOG_LEVEL(INFO, "Construct type hierarchy");
  IRDBModule = IRDB.getModule();
  getAllMetadata(IRDB);
  buildLLVMTypeHierarchy();
}

void LLVMTypeHierarchy_MD::buildLLVMTypeHierarchy() {
  // build the hierarchy for the module
  constructHierarchy();
  // cache the reachable types

  // TODO (max): implement the caching of reachable types
}

void LLVMTypeHierarchy_MD::constructHierarchy() {
  // PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMTypeHierarchy",
  //"Analyze types in module: " << M.getModuleIdentifier());

  // global variable
  for (const auto *Node : MetadataNotes) {
    if (Node->getMetadataID() == llvm::Metadata::DIGlobalVariableKind) {
      // TypeGraph.Vertices.emplace_back(VertexProperties());
      return;
    }

    // structs
    if (Node->getMetadataID() == llvm::Metadata::DICompositeTypeKind) {
      // TypeGraph.Vertices.emplace_back(VertexProperties());
      return;
    }
  }
}

std::set<const llvm::StructType *>
LLVMTypeHierarchy_MD::getSubTypes(const llvm::StructType *Type) {

  // TODO (max): ask fabian how to implement boostless
  // version of TypeVertexMap

  // if (TypeVertexMap.count(Type)) {
  //   return
  //   TypeGraph[TypeVertexMap[Type]].ReachableTypes;
  // }
  return {};
}

std::set<const llvm::StructType *>
LLVMTypeHierarchy_MD::getSuperTypes(const llvm::StructType * /*Type*/) {
  std::set<const llvm::StructType *> ReachableTypes;
  // TODO (philipp): what does this function do?
  return ReachableTypes;
}

const llvm::StructType *
LLVMTypeHierarchy_MD::getType(std::string TypeName) const {
  for (const auto &Vertex : TypeGraph.Vertices) {
    if (Vertex.Type->getName() == TypeName) {
      return Vertex.Type;
    }
  }
  return nullptr;
}

std::set<const llvm::StructType *> LLVMTypeHierarchy_MD::getAllTypes() const {
  std::set<const llvm::StructType *> Types;
  for (const auto &Vertex : TypeGraph.Vertices) {
    Types.insert(Vertex.Type);
  }
  return Types;
}

[[nodiscard]] bool
LLVMTypeHierarchy_MD::hasType(const llvm::StructType *Type) const {
  return false;
}

std::string
LLVMTypeHierarchy_MD::getTypeName(const llvm::StructType *Type) const {
  return Type->getStructName().str();
}

bool LLVMTypeHierarchy_MD::hasVFTable(const llvm::StructType *Type) const {
  if (TypeVFTMap.count(Type)) {
    return !TypeVFTMap.at(Type).empty();
  }
  return false;
}

const LLVMVFTable *
LLVMTypeHierarchy_MD::getVFTable(const llvm::StructType *Type) const {
  if (TypeVFTMap.count(Type)) {
    return &TypeVFTMap.at(Type);
  }
  return nullptr;
}

void LLVMTypeHierarchy_MD::getMetaDataOfFunction(const llvm::Function *F) {
  llvm::SmallVector<std::pair<unsigned, llvm::MDNode *>, 4> MDs;

  F->getAllMetadata(MDs);
  for (auto &CurrentMd : MDs) {
    MetadataNotes.push_back(CurrentMd.second);
  }
}

void LLVMTypeHierarchy_MD::getAllMetadata(const LLVMProjectIRDB &IRDB) {
  FunctionRange AllFunctions = IRDB.getAllFunctions();
  for (const auto *Function : AllFunctions) {
    getMetaDataOfFunction(Function);
  }
}

[[nodiscard]] size_t LLVMTypeHierarchy_MD::size() const {
  return TypeGraph.Vertices.size();
}

[[nodiscard]] bool LLVMTypeHierarchy_MD::empty() const { return size() == 0; }

void LLVMTypeHierarchy_MD::print(llvm::raw_ostream &OS) const {
  OS << "Type Hierarchy:\n";
}

[[nodiscard]] nlohmann::json LLVMTypeHierarchy_MD::getAsJson() const {}

[[nodiscard]] bool
LLVMTypeHierarchy_MD::isSuperType(const llvm::StructType *Type,
                                  const llvm::StructType *SuperType) {}

[[nodiscard]] bool
LLVMTypeHierarchy_MD::isSubType(const llvm::StructType *Type,
                                const llvm::StructType *SubType) {}

} // namespace psr
