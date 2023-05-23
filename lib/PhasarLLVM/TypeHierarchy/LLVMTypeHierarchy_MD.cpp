
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy_MD.h"

#include <llvm-14/llvm/IR/Metadata.h>

namespace psr {

// provide a VertexPropertyWrite to tell boost how to write a vertex
class TypeHierarchyVertexWriter {
public:
  TypeHierarchyVertexWriter(const LLVMTypeHierarchy_MD::bidigraph_t &TyGraph)
      : TyGraph(TyGraph) {}
  template <class VertexOrEdge>
  void operator()(std::ostream &Out, const VertexOrEdge &V) const {
    Out << "[label=\"" << TyGraph[V].getTypeName() << "\"]";
  }

private:
  const LLVMTypeHierarchy_MD::bidigraph_t &TyGraph;
};

LLVMTypeHierarchy_MD::VertexProperties::VertexProperties(
    const llvm::StructType *Type)
    : Type(Type), ReachableTypes({Type}) {}

std::string LLVMTypeHierarchy_MD::VertexProperties::getTypeName() const {
  return Type->getStructName().str();
}

void constructHierarchy(const llvm::Module &M) {
  // TODO:
  /*
  for (unsigned int i = 0; i < Node->getNumOperands(); i++) {
      const llvm::MDOperand &CurrentOp = Node->getOperand(i);
  }

  // What's that? Could be useful. Look up later
  // current_MD.second->printTree();
  */
}

void LLVMTypeHierarchy_MD::getMetaData(llvm::Function &F) {
  llvm::SmallVector<std::pair<unsigned, llvm::MDNode *>, 4> MDs;
  F.getAllMetadata(MDs);
  for (auto &CurrentMd : MDs) {
    MetadataNotes.push_back(CurrentMd.second);
  }
}

} // namespace psr
