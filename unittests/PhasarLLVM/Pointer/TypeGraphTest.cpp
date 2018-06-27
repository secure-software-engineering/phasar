#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/Pointer/TypeGraph.h>
#include <boost/graph/isomorphism.hpp>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;

namespace psr {
  TEST(TypeGraphTest, AddType) {
    ProjectIRDB IRDB({
      "../../../../test/llvm_test_code/basic/two_structs.ll"});
    llvm::Module *M = IRDB.getModule("../../../../test/llvm_test_code/basic/two_structs.ll");

    unsigned int nb_struct = 0;

    TypeGraph tg;
    TypeGraph::graph_t g;

    for ( auto struct_type : M->getIdentifiedStructTypes() ) {
      ASSERT_TRUE(struct_type != nullptr);

      auto node = tg.addType(struct_type);

      ASSERT_TRUE(tg.g[node].name == uniformTypeName(struct_type->getName().str()));
      ASSERT_TRUE(tg.g[node].base_type == struct_type);
      ASSERT_TRUE(tg.g[node].types.size() == 1);
      ASSERT_TRUE(tg.g[node].types.count(struct_type));

      boost::add_vertex(g);

      ASSERT_TRUE(boost::isomorphism(g, tg.g));

      ++nb_struct;
    }

    ASSERT_TRUE(nb_struct >= 2);
  }

  TEST(TypeGraphTest, ReverseTypePropagation) {
    ProjectIRDB IRDB({
      "../../../../test/llvm_test_code/basic/seven_structs.ll"});
    llvm::Module *M = IRDB.getModule("../../../../test/llvm_test_code/basic/seven_structs.ll");

    unsigned int nb_struct = 0;
    llvm::StructType *structA = nullptr,
                     *structB = nullptr,
                     *structC = nullptr,
                     *structD = nullptr,
                     *structE = nullptr;

    TypeGraph tg;

    // Isomorphism to assure that the TypeGraph have the wanted structure
    TypeGraph::graph_t g;

    for ( auto struct_type : M->getIdentifiedStructTypes() ) {
      if ( struct_type ) {
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
            // NB: Will always fail but serve to understand where the error come from
            ASSERT_TRUE( nb_struct < 7);
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
    for ( auto it = tg_edges.first; it != tg_edges.second; ++it ) {
      ++number_edge;

      auto src = boost::source(*it, tg.g);
      auto target = boost::target(*it, tg.g);

      ASSERT_TRUE( number_edge <= 4 );

      ASSERT_TRUE( src == nodeA || src == nodeB || src == nodeC || src == nodeE );
      if ( src == nodeA )
        ASSERT_TRUE( target == nodeB );
      else if ( src == nodeB )
        ASSERT_TRUE( target == nodeC );
      else if ( src == nodeC )
        ASSERT_TRUE( target == nodeD );
      else if ( src == nodeE )
        ASSERT_TRUE( target == nodeB );
    }

    ASSERT_TRUE( number_edge == 4 );
    number_edge = 0; // Avoid stupid mistakes

    // Check that the type are coherent in the graph
    ASSERT_TRUE( tg.g[vertexA].types.count(structA) );
    ASSERT_TRUE( tg.g[vertexA].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexB].types.count(structB) );
    ASSERT_TRUE( tg.g[vertexB].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexC].types.count(structC) );
    ASSERT_TRUE( tg.g[vertexC].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexD].types.count(structD) );
    ASSERT_TRUE( tg.g[vertexD].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexE].types.count(structE) );
    ASSERT_TRUE( tg.g[vertexE].types.size() == 1 );

    tg.reverseTypePropagation(structC);

    // Check that the type are coherent in the graph
    ASSERT_TRUE( tg.g[vertexA].types.count(structA)
              && tg.g[vertexA].types.count(structB)
              && tg.g[vertexA].types.count(structC) );
    ASSERT_TRUE( tg.g[vertexA].types.size() == 3 );
    ASSERT_TRUE( tg.g[vertexB].types.count(structB)
              && tg.g[vertexB].types.count(structC) );
    ASSERT_TRUE( tg.g[vertexB].types.size() == 2 );
    ASSERT_TRUE( tg.g[vertexC].types.count(structC) );
    ASSERT_TRUE( tg.g[vertexC].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexD].types.count(structD) );
    ASSERT_TRUE( tg.g[vertexD].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexE].types.count(structE)
              && tg.g[vertexE].types.count(structB)
              && tg.g[vertexE].types.count(structC) );
    ASSERT_TRUE( tg.g[vertexE].types.size() == 3 );
  }

  TEST(TypeGraphTest, AddLinkSimple) {
    ProjectIRDB IRDB({
      "../../../../test/llvm_test_code/basic/two_structs.ll"});
    llvm::Module *M = IRDB.getModule("../../../../test/llvm_test_code/basic/two_structs.ll");

    unsigned int nb_struct = 0;
    llvm::StructType *structA = nullptr, *structB = nullptr;

    TypeGraph tg;
    TypeGraph::graph_t g;

    for ( auto struct_type : M->getIdentifiedStructTypes() ) {
      if ( struct_type ) {
        switch (nb_struct) {
          case 0:
            structA = struct_type;
            break;
          case 1:
            structB = struct_type;
            break;
          default:
            // NB: Will always fail but serve to understand where the error come from
            ASSERT_TRUE( nb_struct < 2);
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

    for ( auto it = begin; it != end; ++it ) {
      ++number_edge;

      auto src = boost::source(*it, tg.g);
      auto target = boost::target(*it, tg.g);

      ASSERT_TRUE(number_edge == 1);
      ASSERT_TRUE(src == nodeA);
      ASSERT_TRUE(target == nodeB);
    }

    ASSERT_TRUE(number_edge == 1);
  }

  TEST(TypeGraphTest, AddLinkWithSubs) {
    ProjectIRDB IRDB({
      "../../../../test/llvm_test_code/basic/seven_structs.ll"});
    llvm::Module *M = IRDB.getModule("../../../../test/llvm_test_code/basic/seven_structs.ll");

    /* This test use the following hierarchy to test that everything went smoothly
     * when adding some links
     * [5] -> [4] ; [5] -> [3] ;
     * [4] -> [1] ; [3] -> [1] ; [3] -> [2] ;
     *
     * With the following links in each of these typegraphs :
     * [5] : (A) -> (B) ; (D) -> (E)
     * [4] : (F) -> (D)
     * [3] : (B) -> (C)
     * [2] : (C) -> (F)
     * [1] : (E) -> (G)
     */

    unsigned int nb_struct = 0;
    llvm::StructType *structA = nullptr,
                     *structB = nullptr,
                     *structC = nullptr,
                     *structD = nullptr,
                     *structE = nullptr,
                     *structF = nullptr,
                     *structG = nullptr;

    TypeGraph tg1, tg2, tg3, tg4, tg5;

    // Isomorphism to assure that the TypeGraph have the wanted structure
    TypeGraph::graph_t g1, g2, g3, g4, g5;

    for ( auto struct_type : M->getIdentifiedStructTypes() ) {
      if ( struct_type ) {
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
            structF = struct_type;
            break;
          case 6:
            structG = struct_type;
            break;
          default:
            // NB: Will always fail but serve to understand where the error come from
            ASSERT_TRUE( nb_struct < 7);
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
    ASSERT_TRUE(structF != nullptr);
    ASSERT_TRUE(structG != nullptr);

    auto vertexE1 = boost::add_vertex(g1);
    auto vertexG1 = boost::add_vertex(g1);

    auto vertexC2 = boost::add_vertex(g2);
    auto vertexF2 = boost::add_vertex(g2);

    auto vertexB3 = boost::add_vertex(g3);
    auto vertexC3 = boost::add_vertex(g3);
    auto vertexE3 = boost::add_vertex(g3);
    auto vertexF3 = boost::add_vertex(g3);
    auto vertexG3 = boost::add_vertex(g3);

    auto vertexD4 = boost::add_vertex(g4);
    auto vertexE4 = boost::add_vertex(g4);
    auto vertexF4 = boost::add_vertex(g4);
    auto vertexG4 = boost::add_vertex(g4);

    auto vertexA5 = boost::add_vertex(g5);
    auto vertexB5 = boost::add_vertex(g5);
    auto vertexC5 = boost::add_vertex(g5);
    auto vertexD5 = boost::add_vertex(g5);
    auto vertexE5 = boost::add_vertex(g5);
    auto vertexF5 = boost::add_vertex(g5);
    auto vertexG5 = boost::add_vertex(g5);

    auto nodeE1 = tg1.addType(structE);
    auto nodeG1 = tg1.addType(structG);

    auto nodeC2 = tg2.addType(structC);
    auto nodeF2 = tg2.addType(structF);

    auto nodeB3 = tg3.addType(structB);
    auto nodeC3 = tg3.addType(structC);
    auto nodeE3 = tg3.addType(structE);
    auto nodeF3 = tg3.addType(structF);
    auto nodeG3 = tg3.addType(structG);

    auto nodeD4 = tg4.addType(structD);
    auto nodeE4 = tg4.addType(structE);
    auto nodeF4 = tg4.addType(structF);
    auto nodeG4 = tg4.addType(structG);

    auto nodeA5 = tg5.addType(structA);
    auto nodeB5 = tg5.addType(structB);
    auto nodeC5 = tg5.addType(structC);
    auto nodeD5 = tg5.addType(structD);
    auto nodeE5 = tg5.addType(structE);
    auto nodeF5 = tg5.addType(structF);
    auto nodeG5 = tg5.addType(structG);

    // Construct tg5 (without children) and check it has been properly constructed
    tg5.addLink(structA, structB);
    tg5.addLink(structD, structE);

    boost::add_edge(vertexA5, vertexB5, g5);
    boost::add_edge(vertexD5, vertexE5, g5);

    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));

    auto tg5_edges = boost::edges(tg5.g),
         tg4_edges = boost::edges(tg4.g),
         tg3_edges = boost::edges(tg3.g),
         tg2_edges = boost::edges(tg2.g),
         tg1_edges = boost::edges(tg1.g);

    int number_edge5 = 0,
        number_edge4 = 0,
        number_edge3 = 0,
        number_edge2 = 0,
        number_edge1 = 0;

    tg5_edges = boost::edges(tg5.g);
    for ( auto it = tg5_edges.first; it != tg5_edges.second; ++it ) {
      ++number_edge5;

      auto src = boost::source(*it, tg5.g);
      auto target = boost::target(*it, tg5.g);

      ASSERT_TRUE(number_edge5 <= 2);

      ASSERT_TRUE(src == nodeA5 || src == nodeD5);
      if ( src == nodeA5 )
        ASSERT_TRUE( target == nodeB5 );
      else if ( src == nodeD5 )
        ASSERT_TRUE( target == nodeE5 );
    }

    ASSERT_TRUE(number_edge5 == 2);
    number_edge5 = 0; // Avoid stupid mistakes

    // Check that the type are coherent in the graph
    EXPECT_TRUE( tg5.g[vertexA5].types.count(structA)
              && tg5.g[vertexA5].types.count(structB) );
    EXPECT_TRUE( tg5.g[vertexA5].types.size() == 2 );
    EXPECT_TRUE( tg5.g[vertexB5].types.count(structB) );
    EXPECT_TRUE( tg5.g[vertexB5].types.size() == 1 );
    EXPECT_TRUE( tg5.g[vertexC5].types.count(structC) );
    EXPECT_TRUE( tg5.g[vertexC5].types.size() == 1 );
    EXPECT_TRUE( tg5.g[vertexD5].types.count(structD)
              && tg5.g[vertexD5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexD5].types.size() == 2 );
    EXPECT_TRUE( tg5.g[vertexE5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexE5].types.size() == 1 );
    EXPECT_TRUE( tg5.g[vertexF5].types.count(structF) );
    EXPECT_TRUE( tg5.g[vertexF5].types.size() == 1 );
    EXPECT_TRUE( tg5.g[vertexG5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexG5].types.size() == 1 );

    // Add tg4 as a child of tg5 and check if there is no changement other than the
    // addition to the parent_graphs
    tg5.merge(&tg4);

    ASSERT_TRUE( tg4.parent_graphs.count(&tg5) );

    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));
    ASSERT_TRUE(boost::isomorphism(g4, tg4.g));

    //The same for the overall hierarchy
    tg5.merge(&tg3);

    ASSERT_TRUE( tg3.parent_graphs.count(&tg5) );

    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));
    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));

    tg4.merge(&tg1);

    ASSERT_TRUE( tg1.parent_graphs.count(&tg4) );

    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));
    ASSERT_TRUE(boost::isomorphism(g4, tg4.g));
    ASSERT_TRUE(boost::isomorphism(g1, tg1.g));

    tg3.merge(&tg1);

    ASSERT_TRUE( tg1.parent_graphs.count(&tg3) );

    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));
    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));
    ASSERT_TRUE(boost::isomorphism(g1, tg1.g));

    tg3.merge(&tg2);

    ASSERT_TRUE( tg2.parent_graphs.count(&tg3) );

    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));
    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));
    ASSERT_TRUE(boost::isomorphism(g2, tg2.g));

    // Final check that everything is fine before creating the links

    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));
    ASSERT_TRUE(boost::isomorphism(g4, tg4.g));
    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));
    ASSERT_TRUE(boost::isomorphism(g2, tg2.g));
    ASSERT_TRUE(boost::isomorphism(g1, tg1.g));

    // Start adding link and checking the graph while we add the links
    // /!\ CAUTION : This may fail if the algorithm behind the add_link becomes
    // lazy
      // Construct [4]
    tg4.addLink(structF, structD);

    boost::add_edge(vertexF4, vertexD4, g4);
    boost::add_edge(vertexF5, vertexD5, g5);

    ASSERT_TRUE(boost::isomorphism(g4, tg4.g));
    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));

    tg5_edges = edges(tg5.g);
    number_edge5 = 0;

    for ( auto it = tg5_edges.first; it != tg5_edges.second; ++it ) {
      ++number_edge5;

      auto src = boost::source(*it, tg5.g);
      auto target = boost::target(*it, tg5.g);

      ASSERT_TRUE(number_edge5 <= 3);

      ASSERT_TRUE(src == nodeA5 || src == nodeD5 || src == nodeF5 );
      if ( src == nodeA5 )
        ASSERT_TRUE( target == nodeB5 );
      else if ( src == nodeD5 )
        ASSERT_TRUE( target == nodeE5 );
      else if ( src == nodeF5 )
        ASSERT_TRUE( target == nodeD5 );
    }

    ASSERT_TRUE(number_edge5 == 3);
    number_edge5 = 0; // Avoid stupid mistakes

    tg4_edges = edges(tg4.g);
    number_edge4 = 0;

    for ( auto it = tg4_edges.first; it != tg4_edges.second; ++it ) {
      ++number_edge4;

      auto src = boost::source(*it, tg4.g);
      auto target = boost::target(*it, tg4.g);

      ASSERT_TRUE(number_edge4 <= 1);

      ASSERT_TRUE( src == nodeF4 );
      if ( src == nodeF4 )
        ASSERT_TRUE( target == nodeD4 );
    }

    ASSERT_TRUE(number_edge4 == 1);
    number_edge4 = 0; // Avoid stupid mistakes

    EXPECT_TRUE( tg5.g[vertexA5].types.count(structA)
              && tg5.g[vertexA5].types.count(structB) );
    EXPECT_TRUE( tg5.g[vertexA5].types.size() == 2 );
    EXPECT_TRUE( tg5.g[vertexB5].types.count(structB) );
    EXPECT_TRUE( tg5.g[vertexB5].types.size() == 1 );
    EXPECT_TRUE( tg5.g[vertexC5].types.count(structC) );
    EXPECT_TRUE( tg5.g[vertexC5].types.size() == 1 );
    EXPECT_TRUE( tg5.g[vertexD5].types.count(structD)
              && tg5.g[vertexD5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexD5].types.size() == 2 );
    EXPECT_TRUE( tg5.g[vertexE5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexE5].types.size() == 1 );
    EXPECT_TRUE( tg5.g[vertexF5].types.count(structF)
              && tg5.g[vertexF5].types.count(structD)
              && tg5.g[vertexF5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexF5].types.size() == 3 );
    EXPECT_TRUE( tg5.g[vertexG5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexG5].types.size() == 1 );

    EXPECT_TRUE( tg4.g[vertexD4].types.count(structD) );
    EXPECT_TRUE( tg4.g[vertexD4].types.size() == 1 );
    EXPECT_TRUE( tg4.g[vertexE4].types.count(structE) );
    EXPECT_TRUE( tg4.g[vertexE4].types.size() == 1 );
    EXPECT_TRUE( tg4.g[vertexF4].types.count(structF)
              && tg4.g[vertexF4].types.count(structD) );
    EXPECT_TRUE( tg4.g[vertexF4].types.size() == 2 );
    EXPECT_TRUE( tg4.g[vertexG4].types.count(structG) );
    EXPECT_TRUE( tg4.g[vertexG4].types.size() == 1 );

      // Construct [3]
    tg3.addLink(structB, structC);

    boost::add_edge(vertexB3, vertexC3, g3);
    boost::add_edge(vertexB5, vertexC5, g5);

    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));
    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));

    tg5_edges = edges(tg5.g);
    number_edge5 = 0;

    for ( auto it = tg5_edges.first; it != tg5_edges.second; ++it ) {
      ++number_edge5;

      auto src = boost::source(*it, tg5.g);
      auto target = boost::target(*it, tg5.g);

      ASSERT_TRUE(number_edge5 <= 4);

      ASSERT_TRUE(src == nodeA5 || src == nodeD5 || src == nodeF5 || src == nodeB5 );
      if ( src == nodeA5 )
        ASSERT_TRUE( target == nodeB5 );
      else if ( src == nodeD5 )
        ASSERT_TRUE( target == nodeE5 );
      else if ( src == nodeF5 )
        ASSERT_TRUE( target == nodeD5 );
      else if ( src == nodeB5 )
        ASSERT_TRUE( target == nodeC5 );
    }

    ASSERT_TRUE(number_edge5 == 4);
    number_edge5 = 0; // Avoid stupid mistakes

    tg3_edges = edges(tg3.g);
    number_edge3 = 0;

    for ( auto it = tg3_edges.first; it != tg3_edges.second; ++it ) {
      ++number_edge3;

      auto src = boost::source(*it, tg3.g);
      auto target = boost::target(*it, tg3.g);

      ASSERT_TRUE(number_edge3 <= 1);

      ASSERT_TRUE( src == nodeB3 );
      if ( src == nodeB3 )
        ASSERT_TRUE( target == nodeC3 );
    }

    ASSERT_TRUE(number_edge3 == 1);
    number_edge3 = 0; // Avoid stupid mistakes

    EXPECT_TRUE( tg5.g[vertexA5].types.count(structA)
              && tg5.g[vertexA5].types.count(structB)
              && tg5.g[vertexA5].types.count(structC) );
    EXPECT_TRUE( tg5.g[vertexA5].types.size() == 3 );
    EXPECT_TRUE( tg5.g[vertexB5].types.count(structB)
              && tg5.g[vertexB5].types.count(structC) );
    EXPECT_TRUE( tg5.g[vertexB5].types.size() == 2 );
    EXPECT_TRUE( tg5.g[vertexC5].types.count(structC) );
    EXPECT_TRUE( tg5.g[vertexC5].types.size() == 1 );
    EXPECT_TRUE( tg5.g[vertexD5].types.count(structD)
              && tg5.g[vertexD5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexD5].types.size() == 2 );
    EXPECT_TRUE( tg5.g[vertexE5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexE5].types.size() == 1 );
    EXPECT_TRUE( tg5.g[vertexF5].types.count(structF)
              && tg5.g[vertexF5].types.count(structD)
              && tg5.g[vertexF5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexF5].types.size() == 3 );
    EXPECT_TRUE( tg5.g[vertexG5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexG5].types.size() == 1 );

    EXPECT_TRUE( tg3.g[vertexB3].types.count(structB)
              && tg3.g[vertexB3].types.count(structC) );
    EXPECT_TRUE( tg3.g[vertexB3].types.size() == 2 );
    EXPECT_TRUE( tg3.g[vertexC3].types.count(structC) );
    EXPECT_TRUE( tg3.g[vertexC3].types.size() == 1 );
    EXPECT_TRUE( tg3.g[vertexE3].types.count(structE) );
    EXPECT_TRUE( tg3.g[vertexE3].types.size() == 1 );
    EXPECT_TRUE( tg3.g[vertexF3].types.count(structF) );
    EXPECT_TRUE( tg3.g[vertexF3].types.size() == 1 );
    EXPECT_TRUE( tg3.g[vertexG3].types.count(structG) );
    EXPECT_TRUE( tg3.g[vertexG3].types.size() == 1 );

      // Construct [2]
    tg2.addLink(structC, structF);

    boost::add_edge(vertexC2, vertexF2, g2);
    boost::add_edge(vertexC3, vertexF3, g3);
    boost::add_edge(vertexC5, vertexF5, g5);

    ASSERT_TRUE(boost::isomorphism(g2, tg2.g));
    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));
    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));

    tg5_edges = edges(tg5.g);
    number_edge5 = 0;

    for ( auto it = tg5_edges.first; it != tg5_edges.second; ++it ) {
      ++number_edge5;

      auto src = boost::source(*it, tg5.g);
      auto target = boost::target(*it, tg5.g);

      ASSERT_TRUE(number_edge5 <= 5);

      ASSERT_TRUE(src == nodeA5 || src == nodeB5 || src == nodeC5 || src == nodeD5 || src == nodeF5 );
      if ( src == nodeA5 )
        ASSERT_TRUE( target == nodeB5 );
      else if ( src == nodeD5 )
        ASSERT_TRUE( target == nodeE5 );
      else if ( src == nodeF5 )
        ASSERT_TRUE( target == nodeD5 );
      else if ( src == nodeB5 )
        ASSERT_TRUE( target == nodeC5 );
      else if ( src == nodeC5 )
        ASSERT_TRUE( target == nodeF5 );
    }

    ASSERT_TRUE(number_edge5 == 5);
    number_edge5 = 0; // Avoid stupid mistakes

    tg3_edges = edges(tg3.g);
    number_edge3 = 0;

    for ( auto it = tg3_edges.first; it != tg3_edges.second; ++it ) {
      ++number_edge3;

      auto src = boost::source(*it, tg3.g);
      auto target = boost::target(*it, tg3.g);

      ASSERT_TRUE(number_edge3 <= 2);

      ASSERT_TRUE( src == nodeB3 || src == nodeC3 );
      if ( src == nodeB3 )
        ASSERT_TRUE( target == nodeC3 );
      else if ( src == nodeC3 )
        ASSERT_TRUE( target == nodeF3 );
    }

    ASSERT_TRUE(number_edge3 == 2);
    number_edge3 = 0; // Avoid stupid mistakes

    tg2_edges = edges(tg2.g);
    number_edge2 = 0;

    for ( auto it = tg2_edges.first; it != tg2_edges.second; ++it ) {
      ++number_edge2;

      auto src = boost::source(*it, tg2.g);
      auto target = boost::target(*it, tg2.g);

      ASSERT_TRUE(number_edge2 <= 1);

      ASSERT_TRUE( src == nodeC2 );
      if ( src == nodeC2 )
        ASSERT_TRUE( target == nodeF2 );
    }

    ASSERT_TRUE(number_edge2 == 1);
    number_edge2 = 0; // Avoid stupid mistakes

    EXPECT_TRUE( tg5.g[vertexA5].types.count(structA)
              && tg5.g[vertexA5].types.count(structB)
              && tg5.g[vertexA5].types.count(structC)
              && tg5.g[vertexA5].types.count(structF)
              && tg5.g[vertexA5].types.count(structD)
              && tg5.g[vertexA5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexA5].types.size() == 6 );
    EXPECT_TRUE( tg5.g[vertexB5].types.count(structB)
              && tg5.g[vertexB5].types.count(structC)
              && tg5.g[vertexB5].types.count(structF)
              && tg5.g[vertexB5].types.count(structD)
              && tg5.g[vertexB5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexB5].types.size() == 5 );
    EXPECT_TRUE( tg5.g[vertexC5].types.count(structC)
              && tg5.g[vertexC5].types.count(structF)
              && tg5.g[vertexC5].types.count(structD)
              && tg5.g[vertexC5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexC5].types.size() == 4 );
    EXPECT_TRUE( tg5.g[vertexD5].types.count(structD)
              && tg5.g[vertexD5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexD5].types.size() == 2 );
    EXPECT_TRUE( tg5.g[vertexE5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexE5].types.size() == 1 );
    EXPECT_TRUE( tg5.g[vertexF5].types.count(structF)
              && tg5.g[vertexF5].types.count(structD)
              && tg5.g[vertexF5].types.count(structE) );
    EXPECT_TRUE( tg5.g[vertexF5].types.size() == 3 );
    EXPECT_TRUE( tg5.g[vertexG5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexG5].types.size() == 1 );

    EXPECT_TRUE( tg3.g[vertexB3].types.count(structB)
              && tg3.g[vertexB3].types.count(structC)
              && tg3.g[vertexB3].types.count(structF) );
    EXPECT_TRUE( tg3.g[vertexB3].types.size() == 3 );
    EXPECT_TRUE( tg3.g[vertexC3].types.count(structC)
              && tg3.g[vertexB3].types.count(structF) );
    EXPECT_TRUE( tg3.g[vertexC3].types.size() == 2 );
    EXPECT_TRUE( tg3.g[vertexE3].types.count(structE) );
    EXPECT_TRUE( tg3.g[vertexE3].types.size() == 1 );
    EXPECT_TRUE( tg3.g[vertexF3].types.count(structF) );
    EXPECT_TRUE( tg3.g[vertexF3].types.size() == 1 );
    EXPECT_TRUE( tg3.g[vertexG3].types.count(structG) );
    EXPECT_TRUE( tg3.g[vertexG3].types.size() == 1 );

    EXPECT_TRUE( tg2.g[vertexC2].types.count(structC)
              && tg2.g[vertexC2].types.count(structF) );
    EXPECT_TRUE( tg2.g[vertexC2].types.size() == 2 );
    EXPECT_TRUE( tg2.g[vertexF2].types.count(structF) );
    EXPECT_TRUE( tg2.g[vertexF2].types.size() == 1 );

      // Construct [2]
    tg1.addLink(structE, structG);

    boost::add_edge(vertexE1, vertexG1, g1);
    boost::add_edge(vertexE3, vertexG3, g3);
    boost::add_edge(vertexE5, vertexG5, g5);

    ASSERT_TRUE(boost::isomorphism(g1, tg1.g));
    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));
    ASSERT_TRUE(boost::isomorphism(g5, tg5.g));

    tg5_edges = edges(tg5.g);
    number_edge5 = 0;

    for ( auto it = tg5_edges.first; it != tg5_edges.second; ++it ) {
      ++number_edge5;

      auto src = boost::source(*it, tg5.g);
      auto target = boost::target(*it, tg5.g);

      ASSERT_TRUE( number_edge5 <= 6 );

      ASSERT_TRUE( src == nodeA5 || src == nodeB5
                || src == nodeC5 || src == nodeD5
                || src == nodeF5 || src == nodeE5 );
      if ( src == nodeA5 )
        ASSERT_TRUE( target == nodeB5 );
      else if ( src == nodeD5 )
        ASSERT_TRUE( target == nodeE5 );
      else if ( src == nodeF5 )
        ASSERT_TRUE( target == nodeD5 );
      else if ( src == nodeB5 )
        ASSERT_TRUE( target == nodeC5 );
      else if ( src == nodeC5 )
        ASSERT_TRUE( target == nodeF5 );
      else if ( src == nodeE5 )
        ASSERT_TRUE( target == nodeG5 );
    }

    ASSERT_TRUE( number_edge5 == 6 );
    number_edge5 = 0; // Avoid stupid mistakes

    tg4_edges = edges(tg4.g);
    number_edge4 = 0;

    for ( auto it = tg4_edges.first; it != tg4_edges.second; ++it ) {
      ++number_edge4;

      auto src = boost::source(*it, tg4.g);
      auto target = boost::target(*it, tg4.g);

      ASSERT_TRUE(number_edge4 <= 2);

      ASSERT_TRUE( src == nodeF4 || src == nodeE4 );
      if ( src == nodeF4 )
        ASSERT_TRUE( target == nodeD4 );
      else if ( src == nodeE4 )
        ASSERT_TRUE( target == nodeG4 );
    }

    ASSERT_TRUE(number_edge4 == 2);
    number_edge4 = 0; // Avoid stupid mistakes

    tg3_edges = edges(tg3.g);
    number_edge3 = 0;

    for ( auto it = tg3_edges.first; it != tg3_edges.second; ++it ) {
      ++number_edge3;

      auto src = boost::source(*it, tg3.g);
      auto target = boost::target(*it, tg3.g);

      ASSERT_TRUE( number_edge3 <= 3 );

      ASSERT_TRUE( src == nodeB3 || src == nodeC3 || src == nodeE3 );
      if ( src == nodeB3 )
        ASSERT_TRUE( target == nodeC3 );
      else if ( src == nodeC3 )
        ASSERT_TRUE( target == nodeF3 );
      else if ( src == nodeE3 )
        ASSERT_TRUE( target == nodeG3 );
    }

    ASSERT_TRUE( number_edge3 == 3 );
    number_edge3 = 0; // Avoid stupid mistakes

    tg1_edges = edges(tg1.g);
    number_edge1 = 0;

    for ( auto it = tg1_edges.first; it != tg1_edges.second; ++it ) {
      ++number_edge1;

      auto src = boost::source(*it, tg1.g);
      auto target = boost::target(*it, tg1.g);

      ASSERT_TRUE(number_edge1 <= 1);

      ASSERT_TRUE( src == nodeE1 );
      if ( src == nodeE1 )
        ASSERT_TRUE( target == nodeG1 );
    }

    ASSERT_TRUE(number_edge1 == 1);
    number_edge1 = 0; // Avoid stupid mistakes

    EXPECT_TRUE( tg5.g[vertexA5].types.count(structA)
              && tg5.g[vertexA5].types.count(structB)
              && tg5.g[vertexA5].types.count(structC)
              && tg5.g[vertexA5].types.count(structF)
              && tg5.g[vertexA5].types.count(structD)
              && tg5.g[vertexA5].types.count(structE)
              && tg5.g[vertexA5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexA5].types.size() == 7 );
    EXPECT_TRUE( tg5.g[vertexB5].types.count(structB)
              && tg5.g[vertexB5].types.count(structC)
              && tg5.g[vertexB5].types.count(structF)
              && tg5.g[vertexB5].types.count(structD)
              && tg5.g[vertexB5].types.count(structE)
              && tg5.g[vertexB5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexB5].types.size() == 6 );
    EXPECT_TRUE( tg5.g[vertexC5].types.count(structC)
              && tg5.g[vertexC5].types.count(structF)
              && tg5.g[vertexC5].types.count(structD)
              && tg5.g[vertexC5].types.count(structE)
              && tg5.g[vertexC5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexC5].types.size() == 5 );
    EXPECT_TRUE( tg5.g[vertexD5].types.count(structD)
              && tg5.g[vertexD5].types.count(structE)
              && tg5.g[vertexD5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexD5].types.size() == 3 );
    EXPECT_TRUE( tg5.g[vertexE5].types.count(structE)
              && tg5.g[vertexE5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexE5].types.size() == 2 );
    EXPECT_TRUE( tg5.g[vertexF5].types.count(structF)
              && tg5.g[vertexF5].types.count(structD)
              && tg5.g[vertexF5].types.count(structE)
              && tg5.g[vertexF5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexF5].types.size() == 4 );
    EXPECT_TRUE( tg5.g[vertexG5].types.count(structG) );
    EXPECT_TRUE( tg5.g[vertexG5].types.size() == 1 );

    EXPECT_TRUE( tg4.g[vertexD4].types.count(structD) );
    EXPECT_TRUE( tg4.g[vertexD4].types.size() == 1 );
    EXPECT_TRUE( tg4.g[vertexE4].types.count(structE)
              && tg4.g[vertexE4].types.count(structG) );
    EXPECT_TRUE( tg4.g[vertexE4].types.size() == 2 );
    EXPECT_TRUE( tg4.g[vertexF4].types.count(structF)
              && tg4.g[vertexF4].types.count(structD) );
    EXPECT_TRUE( tg4.g[vertexF4].types.size() == 2 );
    EXPECT_TRUE( tg4.g[vertexG4].types.count(structG) );
    EXPECT_TRUE( tg4.g[vertexG4].types.size() == 1 );

    EXPECT_TRUE( tg3.g[vertexB3].types.count(structB)
              && tg3.g[vertexB3].types.count(structC)
              && tg3.g[vertexB3].types.count(structF) );
    EXPECT_TRUE( tg3.g[vertexB3].types.size() == 3 );
    EXPECT_TRUE( tg3.g[vertexC3].types.count(structC)
              && tg3.g[vertexB3].types.count(structF) );
    EXPECT_TRUE( tg3.g[vertexC3].types.size() == 2 );
    EXPECT_TRUE( tg3.g[vertexE3].types.count(structE)
              && tg3.g[vertexE3].types.count(structG) );
    EXPECT_TRUE( tg3.g[vertexE3].types.size() == 2 );
    EXPECT_TRUE( tg3.g[vertexF3].types.count(structF) );
    EXPECT_TRUE( tg3.g[vertexF3].types.size() == 1 );
    EXPECT_TRUE( tg3.g[vertexG3].types.count(structG) );
    EXPECT_TRUE( tg3.g[vertexG3].types.size() == 1 );

    EXPECT_TRUE( tg1.g[vertexE1].types.count(structE)
              && tg1.g[vertexE1].types.count(structG) );
    EXPECT_TRUE( tg1.g[vertexE1].types.size() == 2 );
    EXPECT_TRUE( tg1.g[vertexG1].types.count(structG) );
    EXPECT_TRUE( tg1.g[vertexG1].types.size() == 1 );
  }

  TEST(TypeGraphTest, AddLinkWithRecursion) {
    ProjectIRDB IRDB({
      "../../../../test/llvm_test_code/basic/seven_structs.ll"});
    llvm::Module *M = IRDB.getModule("../../../../test/llvm_test_code/basic/seven_structs.ll");

    /* This test use the following hierarchies to test that everything went smoothly
     * when adding some links with recursive structure
     * [1] -> [1]
     * [2] -> [3] ; [3] -> [2]
     *
     * With the following links in each of these typegraphs :
     * [3] : (B) -> (C)
     * [2] : (A) -> (B)
     * [1] : (A) -> (B) ; (C) -> (D)
     */

    unsigned int nb_struct = 0;
    llvm::StructType *structA = nullptr,
                     *structB = nullptr,
                     *structC = nullptr,
                     *structD = nullptr;

    TypeGraph tg1, tg2, tg3;

    // Isomorphism to assure that the TypeGraph have the wanted structure
    TypeGraph::graph_t g1, g2, g3;

    for ( auto struct_type : M->getIdentifiedStructTypes() ) {
      if ( struct_type ) {
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
            break;
          case 5:
            break;
          case 6:
            break;
          default:
            // NB: Will always fail but serve to understand where the error comes from
            ASSERT_TRUE( nb_struct < 7);
            break;
        }

        ++nb_struct;
      }
    }

    ASSERT_TRUE(nb_struct >= 4);
    ASSERT_TRUE(structA != nullptr);
    ASSERT_TRUE(structB != nullptr);
    ASSERT_TRUE(structC != nullptr);
    ASSERT_TRUE(structD != nullptr);

    auto vertexA1 = boost::add_vertex(g1);
    auto vertexB1 = boost::add_vertex(g1);
    auto vertexC1 = boost::add_vertex(g1);
    auto vertexD1 = boost::add_vertex(g1);

    auto vertexA2 = boost::add_vertex(g2);
    auto vertexB2 = boost::add_vertex(g2);
    auto vertexC2 = boost::add_vertex(g2);

    auto vertexA3 = boost::add_vertex(g3);
    auto vertexB3 = boost::add_vertex(g3);
    auto vertexC3 = boost::add_vertex(g3);

    auto nodeA1 = tg1.addType(structA);
    auto nodeB1 = tg1.addType(structB);
    auto nodeC1 = tg1.addType(structC);
    auto nodeD1 = tg1.addType(structD);

    auto nodeA2 = tg2.addType(structA);
    auto nodeB2 = tg2.addType(structB);
    auto nodeC2 = tg2.addType(structC);

    auto nodeA3 = tg3.addType(structA);
    auto nodeB3 = tg3.addType(structB);
    auto nodeC3 = tg3.addType(structC);

    auto tg3_edges = boost::edges(tg3.g),
         tg2_edges = boost::edges(tg2.g),
         tg1_edges = boost::edges(tg1.g);

    int number_edge3 = 0,
        number_edge2 = 0,
        number_edge1 = 0;

    // Create [1] -> [1]
    tg1.merge(&tg1);

    ASSERT_TRUE( tg1.parent_graphs.count(&tg1) );

    ASSERT_TRUE(boost::isomorphism(g1, tg1.g));

    // Create [2] -> [3] ; [3] -> [2]
    tg2.merge(&tg3);
    tg3.merge(&tg2);

    ASSERT_TRUE( tg3.parent_graphs.count(&tg2) );
    ASSERT_TRUE( tg2.parent_graphs.count(&tg3) );

    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));
    ASSERT_TRUE(boost::isomorphism(g2, tg2.g));

    // Final check that everything is fine before creating the links

    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));
    ASSERT_TRUE(boost::isomorphism(g2, tg2.g));
    ASSERT_TRUE(boost::isomorphism(g1, tg1.g));

    // Start adding link and checking the graph while we add the links
    // /!\ CAUTION : This may fail if the algorithm behind the add_link becomes
    // lazy
      // Construct [1]
    tg1.addLink(structA, structB);
    tg1.addLink(structC, structD);

    boost::add_edge(vertexA1, vertexB1, g1);
    boost::add_edge(vertexC1, vertexD1, g1);

    ASSERT_TRUE(boost::isomorphism(g1, tg1.g));

    tg1_edges = edges(tg1.g);
    number_edge1 = 0;

    for ( auto it = tg1_edges.first; it != tg1_edges.second; ++it ) {
      ++number_edge1;

      auto src = boost::source(*it, tg1.g);
      auto target = boost::target(*it, tg1.g);

      ASSERT_TRUE(number_edge1 <= 2);

      ASSERT_TRUE(src == nodeA1 || src == nodeC1 );
      if ( src == nodeA1 )
        ASSERT_TRUE( target == nodeB1 );
      else if ( src == nodeC1 )
        ASSERT_TRUE( target == nodeD1 );
    }

    ASSERT_TRUE(number_edge1 == 2);
    number_edge1 = 0; // Avoid stupid mistakes

    EXPECT_TRUE( tg1.g[vertexA1].types.count(structA)
              && tg1.g[vertexA1].types.count(structB) );
    EXPECT_TRUE( tg1.g[vertexA1].types.size() == 2 );
    EXPECT_TRUE( tg1.g[vertexB1].types.count(structB) );
    EXPECT_TRUE( tg1.g[vertexB1].types.size() == 1 );
    EXPECT_TRUE( tg1.g[vertexC1].types.count(structC)
              && tg1.g[vertexC1].types.count(structD) );
    EXPECT_TRUE( tg1.g[vertexC1].types.size() == 2 );
    EXPECT_TRUE( tg1.g[vertexD1].types.count(structD) );
    EXPECT_TRUE( tg1.g[vertexD1].types.size() == 1 );

      // Construct [2]
    tg2.addLink(structA, structB);

    boost::add_edge(vertexA2, vertexB2, g2);
    boost::add_edge(vertexA3, vertexB3, g3);

    ASSERT_TRUE(boost::isomorphism(g2, tg2.g));
    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));

    tg2_edges = edges(tg2.g);
    number_edge2 = 0;

    for ( auto it = tg2_edges.first; it != tg2_edges.second; ++it ) {
      ++number_edge2;

      auto src = boost::source(*it, tg2.g);
      auto target = boost::target(*it, tg2.g);

      ASSERT_TRUE( number_edge2 <= 1 );

      ASSERT_TRUE( src == nodeA2 );
      if ( src == nodeA2 )
        ASSERT_TRUE( target == nodeB2 );
    }

    ASSERT_TRUE( number_edge2 == 1 );
    number_edge2 = 0; // Avoid stupid mistakes

    tg3_edges = edges(tg3.g);
    number_edge3 = 0;

    for ( auto it = tg3_edges.first; it != tg3_edges.second; ++it ) {
      ++number_edge3;

      auto src = boost::source(*it, tg3.g);
      auto target = boost::target(*it, tg3.g);

      ASSERT_TRUE( number_edge3 <= 1 );

      ASSERT_TRUE( src == nodeA3 );
      if ( src == nodeA3 )
        ASSERT_TRUE( target == nodeB3 );
    }

    ASSERT_TRUE( number_edge3 == 1 );
    number_edge3 = 0; // Avoid stupid mistakes

    EXPECT_TRUE( tg2.g[vertexA2].types.count(structA)
              && tg2.g[vertexA2].types.count(structB) );
    EXPECT_TRUE( tg2.g[vertexA2].types.size() == 2 );
    EXPECT_TRUE( tg2.g[vertexB2].types.count(structB) );
    EXPECT_TRUE( tg2.g[vertexB2].types.size() == 1 );
    EXPECT_TRUE( tg2.g[vertexC2].types.count(structC) );
    EXPECT_TRUE( tg2.g[vertexC2].types.size() == 1 );

    EXPECT_TRUE( tg3.g[vertexA3].types.count(structA)
              && tg3.g[vertexA3].types.count(structB) );
    EXPECT_TRUE( tg3.g[vertexA3].types.size() == 2 );
    EXPECT_TRUE( tg3.g[vertexB3].types.count(structB) );
    EXPECT_TRUE( tg3.g[vertexB3].types.size() == 1 );
    EXPECT_TRUE( tg3.g[vertexC3].types.count(structC) );
    EXPECT_TRUE( tg3.g[vertexC3].types.size() == 1 );

      // Construct [3]
    tg3.addLink(structB, structC);

    boost::add_edge(vertexB2, vertexC2, g2);
    boost::add_edge(vertexB3, vertexC3, g3);

    ASSERT_TRUE(boost::isomorphism(g2, tg2.g));
    ASSERT_TRUE(boost::isomorphism(g3, tg3.g));

    tg2_edges = edges(tg2.g);
    number_edge2 = 0;

    for ( auto it = tg2_edges.first; it != tg2_edges.second; ++it ) {
      ++number_edge2;

      auto src = boost::source(*it, tg2.g);
      auto target = boost::target(*it, tg2.g);

      ASSERT_TRUE( number_edge2 <= 2 );

      ASSERT_TRUE( src == nodeA2 || src == nodeB2 );
      if ( src == nodeA2 )
        ASSERT_TRUE( target == nodeB2 );
      if ( src == nodeB2 )
        ASSERT_TRUE( target == nodeC2 );
    }

    ASSERT_TRUE( number_edge2 == 2 );
    number_edge2 = 0; // Avoid stupid mistakes

    tg3_edges = edges(tg3.g);
    number_edge3 = 0;

    for ( auto it = tg3_edges.first; it != tg3_edges.second; ++it ) {
      ++number_edge3;

      auto src = boost::source(*it, tg3.g);
      auto target = boost::target(*it, tg3.g);

      ASSERT_TRUE( number_edge3 <= 2 );

      ASSERT_TRUE( src == nodeA3 || src == nodeB3 );
      if ( src == nodeA3 )
        ASSERT_TRUE( target == nodeB3 );
      if ( src == nodeB3 )
        ASSERT_TRUE( target == nodeC3 );
    }

    ASSERT_TRUE( number_edge3 == 2 );
    number_edge3 = 0; // Avoid stupid mistakes

    EXPECT_TRUE( tg2.g[vertexA2].types.count(structA)
              && tg2.g[vertexA2].types.count(structB)
              && tg2.g[vertexA2].types.count(structC) );
    EXPECT_TRUE( tg2.g[vertexA2].types.size() == 3 );
    EXPECT_TRUE( tg2.g[vertexB2].types.count(structB)
              && tg2.g[vertexB2].types.count(structC) );
    EXPECT_TRUE( tg2.g[vertexB2].types.size() == 2 );
    EXPECT_TRUE( tg2.g[vertexC2].types.count(structC) );
    EXPECT_TRUE( tg2.g[vertexC2].types.size() == 1 );

    EXPECT_TRUE( tg3.g[vertexA3].types.count(structA)
              && tg3.g[vertexA3].types.count(structB)
              && tg3.g[vertexA3].types.count(structC) );
    EXPECT_TRUE( tg3.g[vertexA3].types.size() == 3 );
    EXPECT_TRUE( tg3.g[vertexB3].types.count(structB)
              && tg3.g[vertexB3].types.count(structC) );
    EXPECT_TRUE( tg3.g[vertexB3].types.size() == 2 );
    EXPECT_TRUE( tg3.g[vertexC3].types.count(structC) );
    EXPECT_TRUE( tg3.g[vertexC3].types.size() == 1 );
  }

  TEST(TypeGraphTest, TypeAggregation) {
    ProjectIRDB IRDB({
      "../../../../test/llvm_test_code/basic/seven_structs.ll"});
    llvm::Module *M = IRDB.getModule("../../../../test/llvm_test_code/basic/seven_structs.ll");

    unsigned int nb_struct = 0;
    llvm::StructType *structA = nullptr,
                     *structB = nullptr,
                     *structC = nullptr,
                     *structD = nullptr,
                     *structE = nullptr;

    TypeGraph tg;

    // Isomorphism to assure that the TypeGraph have the wanted structure
    TypeGraph::graph_t g;

    for ( auto struct_type : M->getIdentifiedStructTypes() ) {
      if ( struct_type ) {
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
            // NB: Will always fail but serve to understand where the error come from
            ASSERT_TRUE( nb_struct < 7);
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
    for ( auto it = tg_edges.first; it != tg_edges.second; ++it ) {
      ++number_edge;

      auto src = boost::source(*it, tg.g);
      auto target = boost::target(*it, tg.g);

      ASSERT_TRUE( number_edge <= 4 );

      ASSERT_TRUE( src == nodeA || src == nodeB || src == nodeC || src == nodeE );
      if ( src == nodeA )
        ASSERT_TRUE( target == nodeB );
      else if ( src == nodeB )
        ASSERT_TRUE( target == nodeC );
      else if ( src == nodeC )
        ASSERT_TRUE( target == nodeD );
      else if ( src == nodeE )
        ASSERT_TRUE( target == nodeB );
    }

    ASSERT_TRUE( number_edge == 4 );
    number_edge = 0; // Avoid stupid mistakes

    // Check that the type are coherent in the graph
    ASSERT_TRUE( tg.g[vertexA].types.count(structA) );
    ASSERT_TRUE( tg.g[vertexA].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexB].types.count(structB) );
    ASSERT_TRUE( tg.g[vertexB].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexC].types.count(structC) );
    ASSERT_TRUE( tg.g[vertexC].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexD].types.count(structD) );
    ASSERT_TRUE( tg.g[vertexD].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexE].types.count(structE) );
    ASSERT_TRUE( tg.g[vertexE].types.size() == 1 );

    tg.aggregateTypes();

    // Check that the type are coherent in the graph
    ASSERT_TRUE( tg.g[vertexA].types.count(structA)
              && tg.g[vertexA].types.count(structB)
              && tg.g[vertexA].types.count(structC)
              && tg.g[vertexA].types.count(structD) );
    ASSERT_TRUE( tg.g[vertexA].types.size() == 4 );
    ASSERT_TRUE( tg.g[vertexB].types.count(structB)
              && tg.g[vertexB].types.count(structC)
              && tg.g[vertexB].types.count(structD));
    ASSERT_TRUE( tg.g[vertexB].types.size() == 3 );
    ASSERT_TRUE( tg.g[vertexC].types.count(structC)
              && tg.g[vertexC].types.count(structD));
    ASSERT_TRUE( tg.g[vertexC].types.size() == 2 );
    ASSERT_TRUE( tg.g[vertexD].types.count(structD) );
    ASSERT_TRUE( tg.g[vertexD].types.size() == 1 );
    ASSERT_TRUE( tg.g[vertexE].types.count(structE)
              && tg.g[vertexE].types.count(structB)
              && tg.g[vertexE].types.count(structC)
              && tg.g[vertexE].types.count(structD));
    ASSERT_TRUE( tg.g[vertexE].types.size() == 4 );
  }

  TEST(TypeGraphTest, Merging) {
    ProjectIRDB IRDB({
      "../../../../test/llvm_test_code/basic/seven_structs.ll"});
    llvm::Module *M = IRDB.getModule("../../../../test/llvm_test_code/basic/seven_structs.ll");

    unsigned int nb_struct = 0;
    llvm::StructType *structA = nullptr,
                     *structB = nullptr,
                     *structC = nullptr;

    TypeGraph tg1, tg2;

    // Isomorphism to assure that the TypeGraph have the wanted structure
    TypeGraph::graph_t g1, g2;

    for ( auto struct_type : M->getIdentifiedStructTypes() ) {
      if ( struct_type ) {
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
            break;
          case 4:
            break;
          case 5:
            break;
          case 6:
            break;
          default:
            // NB: Will always fail but serve to understand where the error come from
            ASSERT_TRUE( nb_struct < 7);
            break;
        }

        ++nb_struct;
      }
    }

    ASSERT_TRUE(nb_struct == 7);
    ASSERT_TRUE(structA != nullptr);
    ASSERT_TRUE(structB != nullptr);
    ASSERT_TRUE(structC != nullptr);

    auto vertexA1 = boost::add_vertex(g1);
    auto vertexB1 = boost::add_vertex(g1);
    auto vertexC1 = boost::add_vertex(g1);

    auto nodeA1 = tg1.addType(structA);
    auto nodeB1 = tg1.addType(structB);
    auto nodeC1 = tg1.addType(structC);

    tg1.addLink(structA, structB);

    boost::add_edge(vertexA1, vertexB1, g1);

    ASSERT_TRUE(boost::isomorphism(g1, tg1.g));

    auto tg1_edges = boost::edges(tg1.g);
    int number_edge1 = 0;

    tg1_edges = boost::edges(tg1.g);
    for ( auto it = tg1_edges.first; it != tg1_edges.second; ++it ) {
      ++number_edge1;

      auto src = boost::source(*it, tg1.g);
      auto target = boost::target(*it, tg1.g);

      ASSERT_TRUE( number_edge1 <= 1 );

      ASSERT_TRUE( src == nodeA1 );
      if ( src == nodeA1 )
        ASSERT_TRUE( target == nodeB1 );
    }

    ASSERT_TRUE( number_edge1 == 1 );
    number_edge1 = 0; // Avoid stupid mistakes

    // Check that the type are coherent in the graph
    ASSERT_TRUE( tg1.g[vertexA1].types.count(structA)
              && tg1.g[vertexA1].types.count(structB) );
    ASSERT_TRUE( tg1.g[vertexA1].types.size() == 2 );
    ASSERT_TRUE( tg1.g[vertexB1].types.count(structB) );
    ASSERT_TRUE( tg1.g[vertexB1].types.size() == 1 );
    ASSERT_TRUE( tg1.g[vertexC1].types.count(structC) );
    ASSERT_TRUE( tg1.g[vertexC1].types.size() == 1 );

    auto vertexA2 = boost::add_vertex(g2);
    auto vertexC2 = boost::add_vertex(g2);

    auto nodeA2 = tg2.addType(structA);
    auto nodeC2 = tg2.addType(structC);

    tg2.addLink(structA, structC);

    boost::add_edge(vertexA2, vertexC2, g2);

    ASSERT_TRUE(boost::isomorphism(g2, tg2.g));

    auto tg2_edges = boost::edges(tg2.g);
    int number_edge2 = 0;

    tg2_edges = boost::edges(tg1.g);
    for ( auto it = tg2_edges.first; it != tg2_edges.second; ++it ) {
      ++number_edge2;

      auto src = boost::source(*it, tg2.g);
      auto target = boost::target(*it, tg2.g);

      ASSERT_TRUE( number_edge2 <= 1 );

      ASSERT_TRUE( src == nodeA2 );
      if ( src == nodeA2 )
        ASSERT_TRUE( target == nodeC2 );
    }

    ASSERT_TRUE( number_edge2 == 1 );
    number_edge2 = 0; // Avoid stupid mistakes

    // Check that the type are coherent in the graph
    ASSERT_TRUE( tg2.g[vertexA2].types.count(structA)
              && tg2.g[vertexA2].types.count(structC) );
    ASSERT_TRUE( tg2.g[vertexA2].types.size() == 2 );
    ASSERT_TRUE( tg2.g[vertexC2].types.count(structC) );
    ASSERT_TRUE( tg2.g[vertexC2].types.size() == 1 );

    tg1.merge(&tg2);
    ASSERT_TRUE( tg2.parent_graphs.count(&tg1) );

    boost::add_edge(vertexA1, vertexC1, g1);

    ASSERT_TRUE(boost::isomorphism(g1, tg1.g));

    tg1_edges = boost::edges(tg1.g);
    number_edge1 = 0;

    tg1_edges = boost::edges(tg1.g);
    for ( auto it = tg1_edges.first; it != tg1_edges.second; ++it ) {
      ++number_edge1;

      auto src = boost::source(*it, tg1.g);
      auto target = boost::target(*it, tg1.g);

      ASSERT_TRUE( number_edge1 <= 2 );

      ASSERT_TRUE( src == nodeA1 );
      if ( src == nodeA1 )
        ASSERT_TRUE( target == nodeB1 || target == nodeC1 );
    }

    ASSERT_TRUE( number_edge1 == 2 );
    number_edge1 = 0; // Avoid stupid mistakes

    // Check that the type are coherent in the graph
    ASSERT_TRUE( tg1.g[vertexA1].types.count(structA)
              && tg1.g[vertexA1].types.count(structB)
              && tg1.g[vertexA1].types.count(structC) );
    ASSERT_TRUE( tg1.g[vertexA1].types.size() == 3 );
    ASSERT_TRUE( tg1.g[vertexB1].types.count(structB) );
    ASSERT_TRUE( tg1.g[vertexB1].types.size() == 1 );
    ASSERT_TRUE( tg1.g[vertexC1].types.count(structC) );
    ASSERT_TRUE( tg1.g[vertexC1].types.size() == 1 );


    ASSERT_TRUE(boost::isomorphism(g2, tg2.g));

    tg2_edges = boost::edges(tg2.g);
    number_edge2 = 0;

    tg2_edges = boost::edges(tg2.g);
    for ( auto it = tg2_edges.first; it != tg2_edges.second; ++it ) {
      ++number_edge2;

      auto src = boost::source(*it, tg2.g);
      auto target = boost::target(*it, tg2.g);

      ASSERT_TRUE( number_edge2 <= 1 );

      ASSERT_TRUE( src == nodeA2 );
      if ( src == nodeA2 )
        ASSERT_TRUE( target == nodeC2 );
    }

    ASSERT_TRUE( number_edge2 == 1 );
    number_edge2 = 0; // Avoid stupid mistakes

    // Check that the type are coherent in the graph
    ASSERT_TRUE( tg2.g[vertexA2].types.count(structA)
              && tg2.g[vertexA2].types.count(structC) );
    ASSERT_TRUE( tg2.g[vertexA2].types.size() == 2 );
    ASSERT_TRUE( tg2.g[vertexC2].types.count(structC) );
    ASSERT_TRUE( tg2.g[vertexC2].types.size() == 1 );
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
