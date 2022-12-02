#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/TypeGraphs/CachedTypeGraph.h"
#include "phasar/PhasarLLVM/Pointer/TypeGraphs/LazyTypeGraph.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Utilities.h"

#include "gtest/gtest.h"

#include "boost/graph/isomorphism.hpp"

#include "TestConfig.h"

#include "llvm/Support/ManagedStatic.h"

using namespace std;
using namespace psr;

namespace psr {

TEST(TypeGraphTest, AddType) {
  ProjectIRDB IRDB({"llvm_test_code/basic/two_structs.ll"});
  llvm::Module *M = IRDB.getModule("llvm_test_code/basic/two_structs.ll");

  unsigned int NbStruct = 0;

  CachedTypeGraph Tg;
  CachedTypeGraph::graph_t G;

  for (auto *StructType : M->getIdentifiedStructTypes()) {
    ASSERT_TRUE(StructType != nullptr);

    auto Node = Tg.addType(StructType);

    ASSERT_TRUE(Tg.G[Node].Name == StructType->getName().str());
    ASSERT_TRUE(Tg.G[Node].BaseType == StructType);
    ASSERT_TRUE(Tg.G[Node].Types.size() == 1);
    ASSERT_TRUE(Tg.G[Node].Types.count(StructType));

    boost::add_vertex(G);

    ASSERT_TRUE(boost::isomorphism(G, Tg.G));

    ++NbStruct;
  }

  ASSERT_TRUE(NbStruct >= 2);
}

TEST(TypeGraphTest, ReverseTypePropagation) {
  ProjectIRDB IRDB({"llvm_test_code/basic/seven_structs.ll"});
  llvm::Module *M = IRDB.getModule("llvm_test_code/basic/seven_structs.ll");

  unsigned int NbStruct = 0;
  llvm::StructType *StructA = nullptr;

  llvm::StructType *StructB = nullptr;

  llvm::StructType *StructC = nullptr;

  llvm::StructType *StructD = nullptr;

  llvm::StructType *StructE = nullptr;

  CachedTypeGraph Tg;

  // Isomorphism to assure that the TypeGraph have the wanted structure
  CachedTypeGraph::graph_t G;

  for (auto *StructType : M->getIdentifiedStructTypes()) {
    if (StructType) {
      switch (NbStruct) {
      case 0:
        StructA = StructType;
        break;
      case 1:
        StructB = StructType;
        break;
      case 2:
        StructC = StructType;
        break;
      case 3:
        StructD = StructType;
        break;
      case 4:
        StructE = StructType;
        break;
      case 5:
        [[fallthrough]];
      case 6:
        break;
      default:
        // NB: Will always fail but serve to understand where the error come
        // from
        ASSERT_TRUE(NbStruct < 7);
        break;
      }

      ++NbStruct;
    }
  }

  ASSERT_TRUE(NbStruct == 7);
  ASSERT_TRUE(StructA != nullptr);
  ASSERT_TRUE(StructB != nullptr);
  ASSERT_TRUE(StructC != nullptr);
  ASSERT_TRUE(StructD != nullptr);
  ASSERT_TRUE(StructE != nullptr);

  auto VertexA = boost::add_vertex(G);
  auto VertexB = boost::add_vertex(G);
  auto VertexC = boost::add_vertex(G);
  auto VertexD = boost::add_vertex(G);
  auto VertexE = boost::add_vertex(G);

  auto NodeA = Tg.addType(StructA);
  auto NodeB = Tg.addType(StructB);
  auto NodeC = Tg.addType(StructC);
  auto NodeD = Tg.addType(StructD);
  auto NodeE = Tg.addType(StructE);

  Tg.addLinkWithoutReversePropagation(StructA, StructB);
  Tg.addLinkWithoutReversePropagation(StructB, StructC);
  Tg.addLinkWithoutReversePropagation(StructC, StructD);
  Tg.addLinkWithoutReversePropagation(StructE, StructB);

  boost::add_edge(VertexA, VertexB, G);
  boost::add_edge(VertexB, VertexC, G);
  boost::add_edge(VertexC, VertexD, G);
  boost::add_edge(VertexE, VertexB, G);

  ASSERT_TRUE(boost::isomorphism(G, Tg.G));

  auto TgEdges = boost::edges(Tg.G);

  int NumberEdge = 0;

  TgEdges = boost::edges(Tg.G);
  for (auto It = TgEdges.first; It != TgEdges.second; ++It) {
    ++NumberEdge;

    auto Src = boost::source(*It, Tg.G);
    auto Target = boost::target(*It, Tg.G);

    ASSERT_TRUE(NumberEdge <= 4);

    ASSERT_TRUE(Src == NodeA || Src == NodeB || Src == NodeC || Src == NodeE);
    if (Src == NodeA) {
      ASSERT_TRUE(Target == NodeB);
    } else if (Src == NodeB) {
      ASSERT_TRUE(Target == NodeC);
    } else if (Src == NodeC) {
      ASSERT_TRUE(Target == NodeD);
    } else if (Src == NodeE) {
      ASSERT_TRUE(Target == NodeB);
    }
  }

  ASSERT_TRUE(NumberEdge == 4);
  NumberEdge = 0; // NOLINT Avoid stupid mistakes

  // Check that the type are coherent in the graph
  ASSERT_TRUE(Tg.G[VertexA].Types.count(StructA));
  ASSERT_TRUE(Tg.G[VertexA].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexB].Types.count(StructB));
  ASSERT_TRUE(Tg.G[VertexB].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexC].Types.count(StructC));
  ASSERT_TRUE(Tg.G[VertexC].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexD].Types.count(StructD));
  ASSERT_TRUE(Tg.G[VertexD].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexE].Types.count(StructE));
  ASSERT_TRUE(Tg.G[VertexE].Types.size() == 1);

  Tg.reverseTypePropagation(StructC);

  // Check that the type are coherent in the graph
  ASSERT_TRUE(Tg.G[VertexA].Types.count(StructA) &&
              Tg.G[VertexA].Types.count(StructB) &&
              Tg.G[VertexA].Types.count(StructC));
  ASSERT_TRUE(Tg.G[VertexA].Types.size() == 3);
  ASSERT_TRUE(Tg.G[VertexB].Types.count(StructB) &&
              Tg.G[VertexB].Types.count(StructC));
  ASSERT_TRUE(Tg.G[VertexB].Types.size() == 2);
  ASSERT_TRUE(Tg.G[VertexC].Types.count(StructC));
  ASSERT_TRUE(Tg.G[VertexC].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexD].Types.count(StructD));
  ASSERT_TRUE(Tg.G[VertexD].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexE].Types.count(StructE) &&
              Tg.G[VertexE].Types.count(StructB) &&
              Tg.G[VertexE].Types.count(StructC));
  ASSERT_TRUE(Tg.G[VertexE].Types.size() == 3);
}

TEST(TypeGraphTest, AddLinkSimple) {
  ProjectIRDB IRDB({"llvm_test_code/basic/two_structs.ll"});
  llvm::Module *M = IRDB.getModule("llvm_test_code/basic/two_structs.ll");

  unsigned int NbStruct = 0;
  llvm::StructType *StructA = nullptr;

  llvm::StructType *StructB = nullptr;

  CachedTypeGraph Tg;
  CachedTypeGraph::graph_t G;

  for (auto *StructType : M->getIdentifiedStructTypes()) {
    if (StructType) {
      switch (NbStruct) {
      case 0:
        StructA = StructType;
        break;
      case 1:
        StructB = StructType;
        break;
      default:
        // NB: Will always fail but serve to understand where the error come
        // from
        ASSERT_TRUE(NbStruct < 2);
        break;
      }

      ++NbStruct;
    }
  }

  ASSERT_TRUE(NbStruct == 2);
  ASSERT_TRUE(StructA != nullptr);
  ASSERT_TRUE(StructB != nullptr);

  auto NodeA = Tg.addType(StructA);
  auto NodeB = Tg.addType(StructB);
  Tg.addLink(StructA, StructB);

  auto VertexA = boost::add_vertex(G);
  auto VertexB = boost::add_vertex(G);

  boost::add_edge(VertexA, VertexB, G);

  ASSERT_TRUE(boost::isomorphism(G, Tg.G));

  auto P = edges(Tg.G);

  auto Begin = P.first;
  auto End = P.second;

  int NumberEdge = 0;

  for (auto It = Begin; It != End; ++It) {
    ++NumberEdge;

    auto Src = boost::source(*It, Tg.G);
    auto Target = boost::target(*It, Tg.G);

    ASSERT_TRUE(NumberEdge == 1);
    ASSERT_TRUE(Src == NodeA);
    ASSERT_TRUE(Target == NodeB);
  }

  ASSERT_TRUE(NumberEdge == 1);
}

TEST(TypeGraphTest, TypeAggregation) {
  ProjectIRDB IRDB({"llvm_test_code/basic/seven_structs.ll"});
  llvm::Module *M = IRDB.getModule("llvm_test_code/basic/seven_structs.ll");

  unsigned int NbStruct = 0;
  llvm::StructType *StructA = nullptr;
  llvm::StructType *StructB = nullptr;
  llvm::StructType *StructC = nullptr;
  llvm::StructType *StructD = nullptr;
  llvm::StructType *StructE = nullptr;

  CachedTypeGraph Tg;

  // Isomorphism to assure that the TypeGraph have the wanted structure
  CachedTypeGraph::graph_t G;

  for (auto *StructType : M->getIdentifiedStructTypes()) {
    if (StructType) {
      switch (NbStruct) {
      case 0:
        StructA = StructType;
        break;
      case 1:
        StructB = StructType;
        break;
      case 2:
        StructC = StructType;
        break;
      case 3:
        StructD = StructType;
        break;
      case 4:
        StructE = StructType;
        break;
      case 5:
        [[fallthrough]];
      case 6:
        break;
      default:
        // NB: Will always fail but serve to understand where the error come
        // from
        ASSERT_TRUE(NbStruct < 7);
        break;
      }

      ++NbStruct;
    }
  }

  ASSERT_TRUE(NbStruct == 7);
  ASSERT_TRUE(StructA != nullptr);
  ASSERT_TRUE(StructB != nullptr);
  ASSERT_TRUE(StructC != nullptr);
  ASSERT_TRUE(StructD != nullptr);
  ASSERT_TRUE(StructE != nullptr);

  auto VertexA = boost::add_vertex(G);
  auto VertexB = boost::add_vertex(G);
  auto VertexC = boost::add_vertex(G);
  auto VertexD = boost::add_vertex(G);
  auto VertexE = boost::add_vertex(G);

  auto NodeA = Tg.addType(StructA);
  auto NodeB = Tg.addType(StructB);
  auto NodeC = Tg.addType(StructC);
  auto NodeD = Tg.addType(StructD);
  auto NodeE = Tg.addType(StructE);

  Tg.addLinkWithoutReversePropagation(StructA, StructB);
  Tg.addLinkWithoutReversePropagation(StructB, StructC);
  Tg.addLinkWithoutReversePropagation(StructC, StructD);
  Tg.addLinkWithoutReversePropagation(StructE, StructB);

  boost::add_edge(VertexA, VertexB, G);
  boost::add_edge(VertexB, VertexC, G);
  boost::add_edge(VertexC, VertexD, G);
  boost::add_edge(VertexE, VertexB, G);

  ASSERT_TRUE(boost::isomorphism(G, Tg.G));

  auto TgEdges = boost::edges(Tg.G);

  int NumberEdge = 0;

  TgEdges = boost::edges(Tg.G);
  for (auto It = TgEdges.first; It != TgEdges.second; ++It) {
    ++NumberEdge;

    auto Src = boost::source(*It, Tg.G);
    auto Target = boost::target(*It, Tg.G);

    ASSERT_TRUE(NumberEdge <= 4);

    ASSERT_TRUE(Src == NodeA || Src == NodeB || Src == NodeC || Src == NodeE);
    if (Src == NodeA) {
      ASSERT_TRUE(Target == NodeB);
    } else if (Src == NodeB) {
      ASSERT_TRUE(Target == NodeC);
    } else if (Src == NodeC) {
      ASSERT_TRUE(Target == NodeD);
    } else if (Src == NodeE) {
      ASSERT_TRUE(Target == NodeB);
    }
  }

  ASSERT_TRUE(NumberEdge == 4);
  NumberEdge = 0; // NOLINT Avoid stupid mistakes

  // Check that the type are coherent in the graph
  ASSERT_TRUE(Tg.G[VertexA].Types.count(StructA));
  ASSERT_TRUE(Tg.G[VertexA].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexB].Types.count(StructB));
  ASSERT_TRUE(Tg.G[VertexB].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexC].Types.count(StructC));
  ASSERT_TRUE(Tg.G[VertexC].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexD].Types.count(StructD));
  ASSERT_TRUE(Tg.G[VertexD].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexE].Types.count(StructE));
  ASSERT_TRUE(Tg.G[VertexE].Types.size() == 1);

  Tg.aggregateTypes();

  // Check that the type are coherent in the graph
  ASSERT_TRUE(Tg.G[VertexA].Types.count(StructA) &&
              Tg.G[VertexA].Types.count(StructB) &&
              Tg.G[VertexA].Types.count(StructC) &&
              Tg.G[VertexA].Types.count(StructD));
  ASSERT_TRUE(Tg.G[VertexA].Types.size() == 4);
  ASSERT_TRUE(Tg.G[VertexB].Types.count(StructB) &&
              Tg.G[VertexB].Types.count(StructC) &&
              Tg.G[VertexB].Types.count(StructD));
  ASSERT_TRUE(Tg.G[VertexB].Types.size() == 3);
  ASSERT_TRUE(Tg.G[VertexC].Types.count(StructC) &&
              Tg.G[VertexC].Types.count(StructD));
  ASSERT_TRUE(Tg.G[VertexC].Types.size() == 2);
  ASSERT_TRUE(Tg.G[VertexD].Types.count(StructD));
  ASSERT_TRUE(Tg.G[VertexD].Types.size() == 1);
  ASSERT_TRUE(Tg.G[VertexE].Types.count(StructE) &&
              Tg.G[VertexE].Types.count(StructB) &&
              Tg.G[VertexE].Types.count(StructC) &&
              Tg.G[VertexE].Types.count(StructD));
  ASSERT_TRUE(Tg.G[VertexE].Types.size() == 4);
}
} // namespace psr
