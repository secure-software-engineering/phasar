#include "phasar/DB/Hexastore.h"
#include "gtest/gtest.h"

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/isomorphism.hpp"
#include <algorithm>
#include <iostream>

using namespace psr;
using namespace std;

TEST(HexastoreTest, QueryBlankFieldEntries) {
  Hexastore H("QueryBlankFieldEntries.sqlite");
  H.put({{"one", "", ""}});
  H.put({{"two", "", ""}});
  H.put({{"", "three", ""}});
  H.put({{"one", "", "four"}});

  // query results
  hs_result FirstRes("one", "", "");
  hs_result SecondRes("one", "", "four");

  auto Result = H.get({{"one", "", ""}});
  ASSERT_EQ(Result.size(), 1);
  ASSERT_EQ(Result[0], FirstRes);

  Result = H.get({{"one", "?", "?"}});
  ASSERT_EQ(Result.size(), 2);
  ASSERT_EQ(Result[0], FirstRes);
  ASSERT_EQ(Result[1], SecondRes);
}

TEST(HexastoreTest, AllQueryTypes) {
  Hexastore H("AllQueryTypes.sqlite");
  H.put({{"mary", "likes", "hexastores"}});
  H.put({{"mary", "likes", "apples"}});
  H.put({{"mary", "hates", "oranges"}});
  H.put({{"peter", "likes", "apples"}});
  H.put({{"peter", "hates", "hexastores"}});
  H.put({{"frank", "admires", "bananas"}});

  std::vector<hs_result> GroundTruth;
  GroundTruth.emplace_back(hs_result("mary", "likes", "hexastores"));
  GroundTruth.emplace_back(hs_result("mary", "likes", "apples"));
  GroundTruth.emplace_back(hs_result("mary", "hates", "oranges"));
  GroundTruth.emplace_back(hs_result("peter", "likes", "apples"));
  GroundTruth.emplace_back(hs_result("peter", "hates", "hexastores"));
  GroundTruth.emplace_back(hs_result("frank", "admires", "bananas"));

  // Does peter hate hexastores? (SPO query in 'spo' tables)
  auto Result = H.get({{"peter", "hates", "hexastores"}});
  ASSERT_EQ(Result.size(), 1);
  ASSERT_EQ(Result[0], GroundTruth[4]);

  // What does Mary like? (SPX query in 'spo' tables)
  Result = H.get({{"mary", "likes", "?"}});
  ASSERT_EQ(Result.size(), 2);
  ASSERT_EQ(Result[0], GroundTruth[0]);
  ASSERT_EQ(Result[1], GroundTruth[1]);

  // What's Franks opinion on bananas? (SXO query in 'sop' tables)
  Result = H.get({{"frank", "?", "bananas"}});
  ASSERT_EQ(Result.size(), 1);
  ASSERT_EQ(Result[0], GroundTruth[5]);

  // Who likes apples? (XPO query in 'pos' tables)
  Result = H.get({{"?", "likes", "apples"}});
  ASSERT_EQ(Result.size(), 2);
  ASSERT_EQ(Result[0], GroundTruth[1]);
  ASSERT_EQ(Result[1], GroundTruth[3]);

  // What's Marry up to? (SXX query in 'spo' tables)
  Result = H.get({{"mary", "?", "?"}});
  ASSERT_EQ(Result.size(), 3);
  ASSERT_EQ(Result[0], GroundTruth[0]);
  ASSERT_EQ(Result[1], GroundTruth[1]);
  ASSERT_EQ(Result[2], GroundTruth[2]);

  // Who likes what? (XPX query in 'pso' tables)
  Result = H.get({{"?", "likes", "?"}});
  ASSERT_EQ(Result.size(), 3);
  ASSERT_EQ(Result[0], GroundTruth[0]);
  ASSERT_EQ(Result[1], GroundTruth[1]);
  ASSERT_EQ(Result[2], GroundTruth[3]);

  // Who has what opinion on apples? (XXO query in 'osp' tables)
  Result = H.get({{"?", "?", "apples"}});
  ASSERT_EQ(Result.size(), 2);
  ASSERT_EQ(Result[0], GroundTruth[1]);
  ASSERT_EQ(Result[1], GroundTruth[3]);

  // All data in the Hexastore? (XXX query in 'spo' tables)
  Result = H.get({{"?", "?", "?"}});
  ASSERT_EQ(Result.size(), 6);
  ASSERT_EQ(Result, GroundTruth);
}

TEST(HexastoreTest, StoreGraphNoEdgeLabels) {
  struct Vertex {
    string name;
    Vertex() = default;
    Vertex(string name) : name(move(name)) {}
  };
  struct Edge {
    string edge_name;
    Edge() = default;
    Edge(string label) : edge_name(move(label)) {}
  };

  using graph_t = boost::adjacency_list<boost::setS, boost::vecS,
                                        boost::undirectedS, Vertex, Edge>;
  using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
  typename boost::graph_traits<graph_t>::edge_iterator ei_start, e_end;

  // graph with unlabeled edges
  graph_t G;

  vertex_t v1 = boost::add_vertex(G);
  G[v1].name = "A";
  vertex_t v2 = boost::add_vertex(G);
  G[v2].name = "B";
  vertex_t v3 = boost::add_vertex(G);
  G[v3].name = "C";
  vertex_t v4 = boost::add_vertex(G);
  G[v4].name = "D";
  vertex_t v5 = boost::add_vertex(G);
  G[v5].name = "X";
  vertex_t v6 = boost::add_vertex(G);
  G[v6].name = "Y";
  vertex_t v7 = boost::add_vertex(G);
  G[v7].name = "Z";

  boost::add_edge(v2, v1, G);
  boost::add_edge(v3, v1, G);
  boost::add_edge(v7, v3, G);
  boost::add_edge(v7, v6, G);
  boost::add_edge(v6, v5, G);
  boost::add_edge(v4, v2, G);

  // std::cout << "Graph G:" << std::endl;
  // boost::print_graph(G, boost::get(&Vertex::name, G));

  Hexastore HS("StoreGraphNoEdgeLabels.sqlite");

  // serialize graph G
  for (tie(ei_start, e_end) = boost::edges(G); ei_start != e_end; ++ei_start) {
    auto source = boost::source(*ei_start, G);
    auto target = boost::target(*ei_start, G);
    HS.put({{G[source].name, "no label", G[target].name}});
  }

  // de-serialize graph G as graph H"
  graph_t H;
  set<string> Recognized;
  map<string, vertex_t> Vertices;

  vector<hs_result> result_set = HS.get({{"?", "no label", "?"}}, 20);

  for (auto entry : result_set) {
    if (Recognized.find(entry.subject) == Recognized.end()) {
      Vertices[entry.subject] = boost::add_vertex(H);
      H[Vertices[entry.subject]].name = entry.subject;
    }
    if (Recognized.find(entry.object) == Recognized.end()) {
      Vertices[entry.object] = boost::add_vertex(H);
      H[Vertices[entry.object]].name = entry.object;
    }
    boost::add_edge(Vertices[entry.subject], Vertices[entry.object], H);
    Recognized.insert(entry.subject);
    Recognized.insert(entry.object);
  }

  // boost::print_graph(H, boost::get(&Vertex::name, H));
  ASSERT_TRUE(boost::isomorphism(G, H));
}

TEST(HexastoreTest, StoreGraphWithEdgeLabels) {
  struct Vertex {
    string name;
    Vertex() = default;
    Vertex(string name) : name(move(name)) {}
  };
  struct Edge {
    string edge_name;
    Edge() = default;
    Edge(string label) : edge_name(move(label)) {}
  };

  using graph_t = boost::adjacency_list<boost::setS, boost::vecS,
                                        boost::undirectedS, Vertex, Edge>;
  using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
  typename boost::graph_traits<graph_t>::edge_iterator ei_start, e_end;

  // graph with labeled edges
  graph_t I;

  vertex_t w1 = boost::add_vertex(I);
  I[w1].name = "A";
  vertex_t w2 = boost::add_vertex(I);
  I[w2].name = "B";
  vertex_t w3 = boost::add_vertex(I);
  I[w3].name = "C";
  vertex_t w4 = boost::add_vertex(I);
  I[w4].name = "D";
  vertex_t w5 = boost::add_vertex(I);
  I[w5].name = "X";
  vertex_t w6 = boost::add_vertex(I);
  I[w6].name = "Y";
  vertex_t w7 = boost::add_vertex(I);
  I[w7].name = "Z";

  boost::add_edge(w2, w1, Edge("one"), I);
  boost::add_edge(w3, w1, Edge("two"), I);
  boost::add_edge(w7, w3, Edge("three"), I);
  boost::add_edge(w7, w6, Edge("four"), I);
  boost::add_edge(w6, w5, Edge("five"), I);
  boost::add_edge(w4, w2, Edge("six"), I);

  // std::cout << "Graph I:" << std::endl;
  // boost::print_graph(I, boost::get(&Vertex::name, I));
  // for (tie(ei_start, e_end) = boost::edges(I); ei_start != e_end; ++ei_start)
  // {
  //   //		auto source = boost::source(*ei_start, I);
  //   //		auto target = boost::target(*ei_start, I);
  //   cout << boost::get(&Edge::edge_name, I, *ei_start) << endl;
  // }

  Hexastore HS("StoreGraphWithEdgeLabels.sqlite");

  // serialize graph I
  for (tie(ei_start, e_end) = boost::edges(I); ei_start != e_end; ++ei_start) {
    auto source = boost::source(*ei_start, I);
    auto target = boost::target(*ei_start, I);
    string edge = boost::get(&Edge::edge_name, I, *ei_start);
    HS.put({{I[source].name, edge, I[target].name}});
  }

  // de-serialize graph I as graph J
  graph_t J;
  set<string> RecognizedVertices;
  map<string, vertex_t> Vertices;
  vector<hs_result> hsi_res = HS.get({{"?", "?", "?"}}, 10);
  for (auto entry : hsi_res) {
    if (RecognizedVertices.find(entry.subject) == RecognizedVertices.end()) {
      Vertices[entry.subject] = boost::add_vertex(J);
      J[Vertices[entry.subject]].name = entry.subject;
    }
    if (RecognizedVertices.find(entry.object) == RecognizedVertices.end()) {
      Vertices[entry.object] = boost::add_vertex(J);
      J[Vertices[entry.object]].name = entry.object;
    }
    RecognizedVertices.insert(entry.subject);
    RecognizedVertices.insert(entry.object);
    boost::add_edge(Vertices[entry.subject], Vertices[entry.object],
                    Edge(entry.predicate), J);
  }

  // boost::print_graph(J, boost::get(&Vertex::name, J));
  // for (tie(ei_start, e_end) = boost::edges(J); ei_start != e_end; ++ei_start)
  // {
  //   cout << boost::get(&Edge::edge_name, J, *ei_start) << endl;
  // }
  ASSERT_TRUE(boost::isomorphism(I, J));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}