#include <algorithm>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/isomorphism.hpp"

#include "gtest/gtest.h"

#include "phasar/DB/Hexastore.h"

using namespace psr;
using namespace std;

TEST(HexastoreTest, QueryBlankFieldEntries) {
  Hexastore H("QueryBlankFieldEntries.sqlite");
  H.put({{"one", "", ""}});
  H.put({{"two", "", ""}});
  H.put({{"", "three", ""}});
  H.put({{"one", "", "four"}});

  // query results
  HSResult FirstRes("one", "", "");
  HSResult SecondRes("one", "", "four");

  auto Result = H.get({{"one", "", ""}});
  ASSERT_EQ(Result.size(), 1U);
  ASSERT_EQ(Result[0], FirstRes);

  Result = H.get({{"one", "?", "?"}});
  ASSERT_EQ(Result.size(), 2U);
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

  std::vector<HSResult> GroundTruth;
  GroundTruth.emplace_back(HSResult("mary", "likes", "hexastores"));
  GroundTruth.emplace_back(HSResult("mary", "likes", "apples"));
  GroundTruth.emplace_back(HSResult("mary", "hates", "oranges"));
  GroundTruth.emplace_back(HSResult("peter", "likes", "apples"));
  GroundTruth.emplace_back(HSResult("peter", "hates", "hexastores"));
  GroundTruth.emplace_back(HSResult("frank", "admires", "bananas"));

  // Does peter hate hexastores? (SPO query in 'spo' tables)
  auto Result = H.get({{"peter", "hates", "hexastores"}});
  ASSERT_EQ(Result.size(), 1U);
  ASSERT_EQ(Result[0], GroundTruth[4]);

  // What does Mary like? (SPX query in 'spo' tables)
  Result = H.get({{"mary", "likes", "?"}});
  ASSERT_EQ(Result.size(), 2U);
  ASSERT_EQ(Result[0], GroundTruth[0]);
  ASSERT_EQ(Result[1], GroundTruth[1]);

  // What's Franks opinion on bananas? (SXO query in 'sop' tables)
  Result = H.get({{"frank", "?", "bananas"}});
  ASSERT_EQ(Result.size(), 1U);
  ASSERT_EQ(Result[0], GroundTruth[5]);

  // Who likes apples? (XPO query in 'pos' tables)
  Result = H.get({{"?", "likes", "apples"}});
  ASSERT_EQ(Result.size(), 2U);
  ASSERT_EQ(Result[0], GroundTruth[1]);
  ASSERT_EQ(Result[1], GroundTruth[3]);

  // What's Marry up to? (SXX query in 'spo' tables)
  Result = H.get({{"mary", "?", "?"}});
  ASSERT_EQ(Result.size(), 3U);
  ASSERT_EQ(Result[0], GroundTruth[0]);
  ASSERT_EQ(Result[1], GroundTruth[1]);
  ASSERT_EQ(Result[2], GroundTruth[2]);

  // Who likes what? (XPX query in 'pso' tables)
  Result = H.get({{"?", "likes", "?"}});
  ASSERT_EQ(Result.size(), 3U);
  ASSERT_EQ(Result[0], GroundTruth[0]);
  ASSERT_EQ(Result[1], GroundTruth[1]);
  ASSERT_EQ(Result[2], GroundTruth[3]);

  // Who has what opinion on apples? (XXO query in 'osp' tables)
  Result = H.get({{"?", "?", "apples"}});
  ASSERT_EQ(Result.size(), 2U);
  ASSERT_EQ(Result[0], GroundTruth[1]);
  ASSERT_EQ(Result[1], GroundTruth[3]);

  // All data in the Hexastore? (XXX query in 'spo' tables)
  Result = H.get({{"?", "?", "?"}});
  ASSERT_EQ(Result.size(), 6U);
  ASSERT_EQ(Result, GroundTruth);
}

TEST(HexastoreTest, StoreGraphNoEdgeLabels) {
  struct Vertex {
    string Name;
    Vertex() = default;
    Vertex(string Name) : Name(move(Name)) {}
  };
  struct Edge {
    string EdgeName;
    Edge() = default;
    Edge(string Label) : EdgeName(move(Label)) {}
  };

  using graph_t = boost::adjacency_list<boost::setS, boost::vecS,
                                        boost::undirectedS, Vertex, Edge>;
  using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
  typename boost::graph_traits<graph_t>::edge_iterator EiStart;

  typename boost::graph_traits<graph_t>::edge_iterator EEnd;

  // graph with unlabeled edges
  graph_t G;

  vertex_t V1 = boost::add_vertex(G);
  G[V1].Name = "A";
  vertex_t V2 = boost::add_vertex(G);
  G[V2].Name = "B";
  vertex_t V3 = boost::add_vertex(G);
  G[V3].Name = "C";
  vertex_t V4 = boost::add_vertex(G);
  G[V4].Name = "D";
  vertex_t V5 = boost::add_vertex(G);
  G[V5].Name = "X";
  vertex_t V6 = boost::add_vertex(G);
  G[V6].Name = "Y";
  vertex_t V7 = boost::add_vertex(G);
  G[V7].Name = "Z";

  boost::add_edge(V2, V1, G);
  boost::add_edge(V3, V1, G);
  boost::add_edge(V7, V3, G);
  boost::add_edge(V7, V6, G);
  boost::add_edge(V6, V5, G);
  boost::add_edge(V4, V2, G);

  // llvm::outs() << "Graph G:" << std::endl;
  // boost::print_graph(G, boost::get(&Vertex::name, G));

  Hexastore HS("StoreGraphNoEdgeLabels.sqlite");

  // serialize graph G
  for (tie(EiStart, EEnd) = boost::edges(G); EiStart != EEnd; ++EiStart) {
    auto Source = boost::source(*EiStart, G);
    auto Target = boost::target(*EiStart, G);
    HS.put({{G[Source].Name, "no label", G[Target].Name}});
  }

  // de-serialize graph G as graph H"
  graph_t H;
  set<string> Recognized;
  map<string, vertex_t> Vertices;

  vector<HSResult> ResultSet = HS.get({{"?", "no label", "?"}}, 20);

  for (const auto &Entry : ResultSet) {
    if (Recognized.find(Entry.Subject) == Recognized.end()) {
      Vertices[Entry.Subject] = boost::add_vertex(H);
      H[Vertices[Entry.Subject]].Name = Entry.Subject;
    }
    if (Recognized.find(Entry.Object) == Recognized.end()) {
      Vertices[Entry.Object] = boost::add_vertex(H);
      H[Vertices[Entry.Object]].Name = Entry.Object;
    }
    boost::add_edge(Vertices[Entry.Subject], Vertices[Entry.Object], H);
    Recognized.insert(Entry.Subject);
    Recognized.insert(Entry.Object);
  }

  // boost::print_graph(H, boost::get(&Vertex::name, H));
  ASSERT_TRUE(boost::isomorphism(G, H));
}

TEST(HexastoreTest, StoreGraphWithEdgeLabels) {
  struct Vertex {
    string Name;
    Vertex() = default;
    Vertex(string Name) : Name(move(Name)) {}
  };
  struct Edge {
    string EdgeName;
    Edge() = default;
    Edge(string Label) : EdgeName(move(Label)) {}
  };

  using graph_t = boost::adjacency_list<boost::setS, boost::vecS,
                                        boost::undirectedS, Vertex, Edge>;
  using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;
  typename boost::graph_traits<graph_t>::edge_iterator EiStart;

  typename boost::graph_traits<graph_t>::edge_iterator EEnd;

  // graph with labeled edges
  graph_t I;

  vertex_t W1 = boost::add_vertex(I);
  I[W1].Name = "A";
  vertex_t W2 = boost::add_vertex(I);
  I[W2].Name = "B";
  vertex_t W3 = boost::add_vertex(I);
  I[W3].Name = "C";
  vertex_t W4 = boost::add_vertex(I);
  I[W4].Name = "D";
  vertex_t W5 = boost::add_vertex(I);
  I[W5].Name = "X";
  vertex_t W6 = boost::add_vertex(I);
  I[W6].Name = "Y";
  vertex_t W7 = boost::add_vertex(I);
  I[W7].Name = "Z";

  boost::add_edge(W2, W1, Edge("one"), I);
  boost::add_edge(W3, W1, Edge("two"), I);
  boost::add_edge(W7, W3, Edge("three"), I);
  boost::add_edge(W7, W6, Edge("four"), I);
  boost::add_edge(W6, W5, Edge("five"), I);
  boost::add_edge(W4, W2, Edge("six"), I);

  // llvm::outs() << "Graph I:" << std::endl;
  // boost::print_graph(I, boost::get(&Vertex::name, I));
  // for (tie(ei_start, e_end) = boost::edges(I); ei_start != e_end; ++ei_start)
  // {
  //   //		auto source = boost::source(*ei_start, I);
  //   //		auto target = boost::target(*ei_start, I);
  //   cout << boost::get(&Edge::edge_name, I, *ei_start) << endl;
  // }

  Hexastore HS("StoreGraphWithEdgeLabels.sqlite");

  // serialize graph I
  for (tie(EiStart, EEnd) = boost::edges(I); EiStart != EEnd; ++EiStart) {
    auto Source = boost::source(*EiStart, I);
    auto Target = boost::target(*EiStart, I);
    string Edge = boost::get(&Edge::EdgeName, I, *EiStart);
    HS.put({{I[Source].Name, Edge, I[Target].Name}});
  }

  // de-serialize graph I as graph J
  graph_t J;
  set<string> RecognizedVertices;
  map<string, vertex_t> Vertices;
  vector<HSResult> HsiRes = HS.get({{"?", "?", "?"}}, 10);
  for (const auto &Entry : HsiRes) {
    if (RecognizedVertices.find(Entry.Subject) == RecognizedVertices.end()) {
      Vertices[Entry.Subject] = boost::add_vertex(J);
      J[Vertices[Entry.Subject]].Name = Entry.Subject;
    }
    if (RecognizedVertices.find(Entry.Object) == RecognizedVertices.end()) {
      Vertices[Entry.Object] = boost::add_vertex(J);
      J[Vertices[Entry.Object]].Name = Entry.Object;
    }
    RecognizedVertices.insert(Entry.Subject);
    RecognizedVertices.insert(Entry.Object);
    boost::add_edge(Vertices[Entry.Subject], Vertices[Entry.Object],
                    Edge(Entry.Predicate), J);
  }

  // boost::print_graph(J, boost::get(&Vertex::name, J));
  // for (tie(ei_start, e_end) = boost::edges(J); ei_start != e_end; ++ei_start)
  // {
  //   cout << boost::get(&Edge::edge_name, J, *ei_start) << endl;
  // }
  ASSERT_TRUE(boost::isomorphism(I, J));
}
