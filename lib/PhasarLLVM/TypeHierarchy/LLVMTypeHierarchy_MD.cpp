#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy_MD.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/Metadata.h"

namespace psr {

LLVMTypeHierarchy_MD::VertexProperties::VertexProperties(
    const llvm::StructType *Type)
    : Type(Type), ReachableTypes({Type}) {}

std::string LLVMTypeHierarchy_MD::VertexProperties::getTypeName() const {
  return Type->getStructName().str();
}

LLVMTypeHierarchy_MD::LLVMTypeHierarchy_MD(LLVMProjectIRDB &IRDB) {
  PHASAR_LOG_LEVEL(INFO, "Construct type hierarchy");

  // TODO (max): getMetadata() here

  buildLLVMTypeHierarchy(*IRDB.getModule());
}

LLVMTypeHierarchy_MD::LLVMTypeHierarchy_MD(const llvm::Module &M) {
  PHASAR_LOG_LEVEL_CAT(INFO, "LLVMTypeHierarchy", "Construct type hierarchy");
  buildLLVMTypeHierarchy(M);
  PHASAR_LOG_LEVEL_CAT(INFO, "LLVMTypeHierarchy", "Finished type hierarchy");
}

void LLVMTypeHierarchy_MD::buildLLVMTypeHierarchy(const llvm::Module &M) {
  // build the hierarchy for the module
  constructHierarchy(M);
  // cache the reachable types

  // TODO (max): implement the caching of reachable types
}

void LLVMTypeHierarchy_MD::constructHierarchy(const llvm::Module &M) {
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMTypeHierarchy",
                       "Analyze types in module: " << M.getModuleIdentifier());
  // store analyzed module
  VisitedModules.insert(&M);

  // TODO (max): go over all metadata nodes and construct hierarchy
}

std::string
LLVMTypeHierarchy_MD::removeStructOrClassPrefix(const llvm::StructType &T) {
  return removeStructOrClassPrefix(T.getName().str());
}

std::string
LLVMTypeHierarchy_MD::removeStructOrClassPrefix(const std::string &TypeName) {
  llvm::StringRef SR(TypeName);
  if (SR.startswith(StructPrefix)) {
    return SR.drop_front(StructPrefix.size()).str();
  }
  if (SR.startswith(ClassPrefix)) {
    return SR.drop_front(ClassPrefix.size()).str();
  }
  return TypeName;
}

std::set<const llvm::StructType *>
LLVMTypeHierarchy_MD::getSubTypes(const llvm::StructType *Type) {

  // TODO (max): ask fabian how to implement boostless version of TypeVertexMap

  // if (TypeVertexMap.count(Type)) {
  //   return TypeGraph[TypeVertexMap[Type]].ReachableTypes;
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

void LLVMTypeHierarchy_MD::getMetaData(llvm::Function &F) {
  llvm::SmallVector<std::pair<unsigned, llvm::MDNode *>, 4> MDs;
  F.getAllMetadata(MDs);
  for (auto &CurrentMd : MDs) {
    MetadataNotes.push_back(CurrentMd.second);
  }
}

} // namespace psr
