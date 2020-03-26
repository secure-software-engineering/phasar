#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/TypeGraphs/CachedTypeGraph.h"
#include "phasar/PhasarLLVM/Pointer/TypeGraphs/LazyTypeGraph.h"
#include "gtest/gtest.h"

#include "boost/graph/isomorphism.hpp"
#include "phasar/Config/Configuration.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

namespace psr {

class TypeGraphTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/";
};

TEST_F(TypeGraphTest, AddType) {
  ProjectIRDB IRDB({pathToLLFiles + "basic/two_structs_cpp.ll"});
  llvm::Module *M = IRDB.getModule(pathToLLFiles + "basic/two_structs_cpp.ll");

  unsigned int nb_struct = 0;

  CachedTypeGraph tg;
  CachedTypeGraph::graph_t g;

  for (auto struct_type : M->getIdentifiedStructTypes()) {
    ASSERT_TRUE(struct_type != nullptr);

    auto node = tg.addType(struct_type);

    ASSERT_TRUE(tg.g[node].name == struct_type->getName().str());
    ASSERT_TRUE(tg.g[node].base_type == struct_type);
    ASSERT_TRUE(tg.g[node].types.size() == 1);
    ASSERT_TRUE(tg.g[node].types.count(struct_type));

    boost::add_vertex(g);

    ASSERT_TRUE(boost::isomorphism(g, tg.g));

    ++nb_struct;
  }

  ASSERT_TRUE(nb_struct >= 2);
}

TEST_F(TypeGraphTest, ReverseTypePropagation) {
  ProjectIRDB IRDB({pathToLLFiles + "basic/seven_structs_cpp.ll"});
  llvm::Module *M =
      IRDB.getModule(pathToLLFiles + "basic/seven_structs_cpp.ll");

  unsigned int nb_struct = 0;
  llvm::StructType *structA = nullptr, *structB = nullptr, *structC = nullptr,
                   *structD = nullptr, *structE = nullptr;

  CachedTypeGraph tg;

  // Isomorphism to assure that the TypeGraph have the wanted structure
  CachedTypeGraph::graph_t g;

  for (auto struct_type : M->getIdentifiedStructTypes()) {
    if (struct_type) {
      switch (nb_struct) {
      case 0:
        structA = struct_type;
        break;
      case 1:
        structB = struct_type;
        break;
      case 2:
        structC = struct_type;
        break;
      case 3:
        structD = struct_type;
        break;
      case 4:
        structE = struct_type;
        break;
      case 5:
        break;
      case 6:
        break;
      default:
        // NB: Will always fail but serve to understand where the error come
        // from
        ASSERT_TRUE(nb_struct < 7);
        break;
      }

      ++nb_struct;
    }
  }

  ASSERT_TRUE(nb_struct == 7);
  ASSERT_TRUE(structA != nullptr);
  ASSERT_TRUE(structB != nullptr);
  ASSERT_TRUE(structC != nullptr);
  ASSERT_TRUE(structD != nullptr);
  ASSERT_TRUE(structE != nullptr);

  auto vertexA = boost::add_vertex(g);
  auto vertexB = boost::add_vertex(g);
  auto vertexC = boost::add_vertex(g);
  auto vertexD = boost::add_vertex(g);
  auto vertexE = boost::add_vertex(g);

  auto nodeA = tg.addType(structA);
  auto nodeB = tg.addType(structB);
  auto nodeC = tg.addType(structC);
  auto nodeD = tg.addType(structD);
  auto nodeE = tg.addType(structE);

  tg.addLinkWithoutReversePropagation(structA, structB);
  tg.addLinkWithoutReversePropagation(structB, structC);
  tg.addLinkWithoutReversePropagation(structC, structD);
  tg.addLinkWithoutReversePropagation(structE, structB);

  boost::add_edge(vertexA, vertexB, g);
  boost::add_edge(vertexB, vertexC, g);
  boost::add_edge(vertexC, vertexD, g);
  boost::add_edge(vertexE, vertexB, g);

  ASSERT_TRUE(boost::isomorphism(g, tg.g));

  auto tg_edges = boost::edges(tg.g);

  int number_edge = 0;

  tg_edges = boost::edges(tg.g);
  for (auto it = tg_edges.first; it != tg_edges.second; ++it) {
    ++number_edge;

    auto src = boost::source(*it, tg.g);
    auto target = boost::target(*it, tg.g);

    ASSERT_TRUE(number_edge <= 4);

    ASSERT_TRUE(src == nodeA || src == nodeB || src == nodeC || src == nodeE);
    if (src == nodeA)
      ASSERT_TRUE(target == nodeB);
    else if (src == nodeB)
      ASSERT_TRUE(target == nodeC);
    else if (src == nodeC)
      ASSERT_TRUE(target == nodeD);
    else if (src == nodeE)
      ASSERT_TRUE(target == nodeB);
  }

  ASSERT_TRUE(number_edge == 4);
  number_edge = 0; // Avoid stupid mistakes

  // Check that the type are coherent in the graph
  ASSERT_TRUE(tg.g[vertexA].types.count(structA));
  ASSERT_TRUE(tg.g[vertexA].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexB].types.count(structB));
  ASSERT_TRUE(tg.g[vertexB].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexC].types.count(structC));
  ASSERT_TRUE(tg.g[vertexC].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexD].types.count(structD));
  ASSERT_TRUE(tg.g[vertexD].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexE].types.count(structE));
  ASSERT_TRUE(tg.g[vertexE].types.size() == 1);

  tg.reverseTypePropagation(structC);

  // Check that the type are coherent in the graph
  ASSERT_TRUE(tg.g[vertexA].types.count(structA) &&
              tg.g[vertexA].types.count(structB) &&
              tg.g[vertexA].types.count(structC));
  ASSERT_TRUE(tg.g[vertexA].types.size() == 3);
  ASSERT_TRUE(tg.g[vertexB].types.count(structB) &&
              tg.g[vertexB].types.count(structC));
  ASSERT_TRUE(tg.g[vertexB].types.size() == 2);
  ASSERT_TRUE(tg.g[vertexC].types.count(structC));
  ASSERT_TRUE(tg.g[vertexC].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexD].types.count(structD));
  ASSERT_TRUE(tg.g[vertexD].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexE].types.count(structE) &&
              tg.g[vertexE].types.count(structB) &&
              tg.g[vertexE].types.count(structC));
  ASSERT_TRUE(tg.g[vertexE].types.size() == 3);
}

TEST_F(TypeGraphTest, AddLinkSimple) {
  ProjectIRDB IRDB({pathToLLFiles + "basic/two_structs_cpp.ll"});
  llvm::Module *M = IRDB.getModule(pathToLLFiles + "basic/two_structs_cpp.ll");

  unsigned int nb_struct = 0;
  llvm::StructType *structA = nullptr, *structB = nullptr;

  CachedTypeGraph tg;
  CachedTypeGraph::graph_t g;

  for (auto struct_type : M->getIdentifiedStructTypes()) {
    if (struct_type) {
      switch (nb_struct) {
      case 0:
        structA = struct_type;
        break;
      case 1:
        structB = struct_type;
        break;
      default:
        // NB: Will always fail but serve to understand where the error come
        // from
        ASSERT_TRUE(nb_struct < 2);
        break;
      }

      ++nb_struct;
    }
  }

  ASSERT_TRUE(nb_struct == 2);
  ASSERT_TRUE(structA != nullptr);
  ASSERT_TRUE(structB != nullptr);

  auto nodeA = tg.addType(structA);
  auto nodeB = tg.addType(structB);
  tg.addLink(structA, structB);

  auto vertexA = boost::add_vertex(g);
  auto vertexB = boost::add_vertex(g);

  boost::add_edge(vertexA, vertexB, g);

  ASSERT_TRUE(boost::isomorphism(g, tg.g));

  auto p = edges(tg.g);

  auto begin = p.first;
  auto end = p.second;

  int number_edge = 0;

  for (auto it = begin; it != end; ++it) {
    ++number_edge;

    auto src = boost::source(*it, tg.g);
    auto target = boost::target(*it, tg.g);

    ASSERT_TRUE(number_edge == 1);
    ASSERT_TRUE(src == nodeA);
    ASSERT_TRUE(target == nodeB);
  }

  ASSERT_TRUE(number_edge == 1);
}

TEST_F(TypeGraphTest, TypeAggregation) {
  ProjectIRDB IRDB({pathToLLFiles + "basic/seven_structs_cpp.ll"});
  llvm::Module *M =
      IRDB.getModule(pathToLLFiles + "basic/seven_structs_cpp.ll");

  unsigned int nb_struct = 0;
  llvm::StructType *structA = nullptr, *structB = nullptr, *structC = nullptr,
                   *structD = nullptr, *structE = nullptr;

  CachedTypeGraph tg;

  // Isomorphism to assure that the TypeGraph have the wanted structure
  CachedTypeGraph::graph_t g;

  for (auto struct_type : M->getIdentifiedStructTypes()) {
    if (struct_type) {
      switch (nb_struct) {
      case 0:
        structA = struct_type;
        break;
      case 1:
        structB = struct_type;
        break;
      case 2:
        structC = struct_type;
        break;
      case 3:
        structD = struct_type;
        break;
      case 4:
        structE = struct_type;
        break;
      case 5:
        break;
      case 6:
        break;
      default:
        // NB: Will always fail but serve to understand where the error come
        // from
        ASSERT_TRUE(nb_struct < 7);
        break;
      }

      ++nb_struct;
    }
  }

  ASSERT_TRUE(nb_struct == 7);
  ASSERT_TRUE(structA != nullptr);
  ASSERT_TRUE(structB != nullptr);
  ASSERT_TRUE(structC != nullptr);
  ASSERT_TRUE(structD != nullptr);
  ASSERT_TRUE(structE != nullptr);

  auto vertexA = boost::add_vertex(g);
  auto vertexB = boost::add_vertex(g);
  auto vertexC = boost::add_vertex(g);
  auto vertexD = boost::add_vertex(g);
  auto vertexE = boost::add_vertex(g);

  auto nodeA = tg.addType(structA);
  auto nodeB = tg.addType(structB);
  auto nodeC = tg.addType(structC);
  auto nodeD = tg.addType(structD);
  auto nodeE = tg.addType(structE);

  tg.addLinkWithoutReversePropagation(structA, structB);
  tg.addLinkWithoutReversePropagation(structB, structC);
  tg.addLinkWithoutReversePropagation(structC, structD);
  tg.addLinkWithoutReversePropagation(structE, structB);

  boost::add_edge(vertexA, vertexB, g);
  boost::add_edge(vertexB, vertexC, g);
  boost::add_edge(vertexC, vertexD, g);
  boost::add_edge(vertexE, vertexB, g);

  ASSERT_TRUE(boost::isomorphism(g, tg.g));

  auto tg_edges = boost::edges(tg.g);

  int number_edge = 0;

  tg_edges = boost::edges(tg.g);
  for (auto it = tg_edges.first; it != tg_edges.second; ++it) {
    ++number_edge;

    auto src = boost::source(*it, tg.g);
    auto target = boost::target(*it, tg.g);

    ASSERT_TRUE(number_edge <= 4);

    ASSERT_TRUE(src == nodeA || src == nodeB || src == nodeC || src == nodeE);
    if (src == nodeA)
      ASSERT_TRUE(target == nodeB);
    else if (src == nodeB)
      ASSERT_TRUE(target == nodeC);
    else if (src == nodeC)
      ASSERT_TRUE(target == nodeD);
    else if (src == nodeE)
      ASSERT_TRUE(target == nodeB);
  }

  ASSERT_TRUE(number_edge == 4);
  number_edge = 0; // Avoid stupid mistakes

  // Check that the type are coherent in the graph
  ASSERT_TRUE(tg.g[vertexA].types.count(structA));
  ASSERT_TRUE(tg.g[vertexA].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexB].types.count(structB));
  ASSERT_TRUE(tg.g[vertexB].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexC].types.count(structC));
  ASSERT_TRUE(tg.g[vertexC].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexD].types.count(structD));
  ASSERT_TRUE(tg.g[vertexD].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexE].types.count(structE));
  ASSERT_TRUE(tg.g[vertexE].types.size() == 1);

  tg.aggregateTypes();

  // Check that the type are coherent in the graph
  ASSERT_TRUE(tg.g[vertexA].types.count(structA) &&
              tg.g[vertexA].types.count(structB) &&
              tg.g[vertexA].types.count(structC) &&
              tg.g[vertexA].types.count(structD));
  ASSERT_TRUE(tg.g[vertexA].types.size() == 4);
  ASSERT_TRUE(tg.g[vertexB].types.count(structB) &&
              tg.g[vertexB].types.count(structC) &&
              tg.g[vertexB].types.count(structD));
  ASSERT_TRUE(tg.g[vertexB].types.size() == 3);
  ASSERT_TRUE(tg.g[vertexC].types.count(structC) &&
              tg.g[vertexC].types.count(structD));
  ASSERT_TRUE(tg.g[vertexC].types.size() == 2);
  ASSERT_TRUE(tg.g[vertexD].types.count(structD));
  ASSERT_TRUE(tg.g[vertexD].types.size() == 1);
  ASSERT_TRUE(tg.g[vertexE].types.count(structE) &&
              tg.g[vertexE].types.count(structB) &&
              tg.g[vertexE].types.count(structC) &&
              tg.g[vertexE].types.count(structD));
  ASSERT_TRUE(tg.g[vertexE].types.size() == 4);
}
} // namespace psr

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto res = RUN_ALL_TESTS();
  llvm::llvm_shutdown();
  return res;
}
