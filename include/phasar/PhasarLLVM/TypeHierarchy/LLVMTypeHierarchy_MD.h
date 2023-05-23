
#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMTYPEHIERARCHY_MD_H_
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMTYPEHIERARCHY_MD_H_

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/TypeHierarchy/TypeHierarchy.h"

#include "llvm/ADT/StringRef.h"

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_traits.hpp"
#include "gtest/gtest_prod.h"
#include "nlohmann/json.hpp"

#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <llvm-14/llvm/IR/DerivedTypes.h>
#include <llvm-14/llvm/IR/Module.h>

namespace psr {

class LLVMTypeHierarchy_MD
    : public TypeHierarchy<const llvm::StructType *, const llvm::Function *> {
public:
  struct VertexProperties {
    VertexProperties() = default;
    VertexProperties(const llvm::StructType *Type);

    [[nodiscard]] std::string getTypeName() const;

    const llvm::StructType *Type = nullptr;
    std::optional<LLVMVFTable> VFT = std::nullopt;
    std::set<const llvm::StructType *> ReachableTypes;
  };

  /// Edges in the class hierarchy graph doesn't hold any additional
  /// information.
  struct EdgeProperties {
    EdgeProperties() = default;
  };

  /**
   * @brief Constructs the actual class hierarchy graph.
   * @param M LLVM module
   *
   * Extracts new information from the given module and adds new vertices
   * and edges accordingly to the type hierarchy graph.
   */
  void constructHierarchy(const llvm::Module &M);

  /// Data structure holding the class hierarchy graph.
  using bidigraph_t =
      boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS,
                            VertexProperties, EdgeProperties>;

private:
  bidigraph_t TypeGraph;
  std::vector<llvm::MDNode *> MetadataNotes;

  /**
   * 	@brief Prints the class hierarchy to an ostream in dot format.
   * 	@param an outputstream
   */
  void printAsDot(llvm::raw_ostream &OS = llvm::outs()) const;

  /**
   * @brief Extracts the metadata from a LLVM function
   * @param F LLVM function
   */
  void getMetaData(llvm::Function &F);
};

} // namespace psr

#endif
