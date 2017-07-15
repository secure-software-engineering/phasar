#include <iostream>
#include <array>
#include <algorithm>
#include <assert.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
#include "Hexastore.hh"

using namespace std;
using namespace hexastore;

int hs_empty_fields_test() {
  hexastore::Hexastore h("test1.sqlite");
  // init with some stuff
  h.put({{"one", "", ""}});
  h.put({{"two", "", ""}});
  h.put({{"", "three", ""}});
  h.put({{"one", "", "four"}});
  // queries
  cout << "one:" << "\n";
  auto result = h.get({{"one", "", ""}});
  for_each(result.begin(), result.end(), [](hs_result r){
    cout << r << endl;
  });
  cout << "\n";
  result = h.get({{"one", "?", "?"}});
  for_each(result.begin(), result.end(), [](hs_result r){
    cout << r << endl;
  });
  return 0;
}

int hs_more_tests() {
  hexastore::Hexastore h("test.sqlite");
  // init with some stuff
  h.put({{"mary", "likes", "hexastores"}});
  h.put({{"mary", "likes", "apples"}});
  h.put({{"mary", "hates", "oranges"}});
  h.put({{"peter", "likes", "apples"}});
  h.put({{"peter", "hates", "hexastores"}});
  h.put({{"frank", "admires", "bananas"}});
  //query SPO (spo tables)
  cout << "Does peter hate hexastores?" << "\n";
  auto result = h.get({{"peter", "hates", "hexastores"}});
  for_each(result.begin(), result.end(), [](hs_result r){
    cout << r << endl;
  });
  //query SPX (spo tables)
  cout << "\n";
  cout << "What does Mary like?" << "\n";
  result = h.get({{"mary", "likes", "?"}});
  for_each(result.begin(), result.end(), [](hs_result r){
    cout << r << endl;
  });
  //query SXO (sop tables)
  cout << "\n";
  cout << "What's Franks opinion on bananas?" << "\n";
  result = h.get({{"frank", "?", "bananas"}});
  for_each(result.begin(), result.end(), [](hs_result r){
    cout << r << endl;
  });
  //query XPO (pos tables)
  cout << "\n";
  cout << "Who likes apples?" << "\n";
  result = h.get({{"?", "likes", "apples"}});
  for_each(result.begin(), result.end(), [](hs_result r){
    cout << r << endl;
  });
  //query SXX (spo tables)
  cout << "\n";
  cout << "What's Marry up to?" << "\n";
  result = h.get({{"mary", "?", "?"}});
  for_each(result.begin(), result.end(), [](hs_result r){
    cout << r << endl;
  });
  //query XPX (pso tables)
  cout << "\n";
  cout << "Who likes what?" << "\n";
  result = h.get({{"?", "likes", "?"}});
  for_each(result.begin(), result.end(), [](hs_result r){
    cout << r << endl;
  });
  //query XXO (osp tables)
  cout << "\n";
  cout << "Who has what opinion on apples?" << "\n";
  result = h.get({{"?", "?", "apples"}});
  for_each(result.begin(), result.end(), [](hs_result r){
    cout << r << endl;
  });
  //query XXX (spo tables)
  cout << "\n";
  cout << "All data in the Hexastore:" << "\n";
  result = h.get({{"?", "?", "?"}});
  for_each(result.begin(), result.end(), [](hs_result r){
    cout << r << endl;
  });
  return 0;
}

int hs_test_main() {
  hexastore::Hexastore h("test.sqlite");
  // init with some stuff
  h.put({{"mary", "likes", "hexastores"}});
  h.put({{"mary", "likes", "apples"}});
  h.put({{"peter", "likes", "apples"}});
  h.put({{"peter", "hates", "hexastores"}});
  h.put({{"frank", "admires", "bananas"}});
  //query some stuff
  cout << "Who likes what?" << "\n";
  auto result = h.get({{"?", "likes", "?"}});
  for_each(result.begin(), result.end(), [](hs_result r){
	  cout << r << endl;
  });
  cout << "\n";
  cout << "What does peter hate?" << "\n";
  result = h.get({{"peter", "hates", "?"}});
  for_each(result.begin(), result.end(), [](hs_result r){
	  cout << r << endl;
  });
  cout << "\n";
  cout << "Who admires something?" << "\n";
  result = h.get({{"?", "admires", "?"}});
  for_each(result.begin(), result.end(), [](hs_result r){
	  cout << r << endl;
  });
  return 0;
}

void hs_serialization_test() {
	// making boost::graph persistent using hexastores
	struct Vertex {
		string name;
		Vertex() = default;
		Vertex(string name) : name(name) {}
	};

	struct Edge {
		string edge_name;
		Edge() = default;
		Edge(string label) : edge_name(label) {}
	};

	// define stuff for an undirected graph
	typedef boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS,
			Vertex, Edge> graph_t;
	typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;
//	typedef boost::graph_traits<graph_t>::edge_descriptor edge_t;
//	typedef boost::graph_traits<graph_t>::vertex_iterator vertex_iterator_t;

//	// define stuff for a directed graph
//	typedef boost::adjacency_list<boost::setS, boost::vecS, boost::directedS,
//			Vertex, Edge> digraph_t;
//	typedef boost::graph_traits<digraph_t>::vertex_descriptor divertext_t;
//	typedef boost::graph_traits<digraph_t>::edge_descriptor diedge_t;
//	typedef boost::graph_traits<digraph_t>::vertex_iterator divertex_iterator_t;

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
	std::cout << "Graph G:" << std::endl;
	boost::print_graph(G, boost::get(&Vertex::name, G));
	hexastore::Hexastore hs("my_boost_graph.sqlite");
	cout << "serialize G\n";
	typename boost::graph_traits<graph_t>::edge_iterator ei_start, e_end;
	for (tie(ei_start, e_end) = boost::edges(G); ei_start != e_end; ++ei_start) {
		auto source = boost::source(*ei_start, G);
		auto target = boost::target(*ei_start, G);
		hs.put({{ G[source].name, "no label", G[target].name}});
	}
	cout << "de-serialize H\n";
	graph_t H;
	set<string> recognized;
	map<string, vertex_t> vertices;
	vector<hexastore::hs_result> result_set = hs.get({{ "?", "no label", "?" }}, 20);
	for (auto entry : result_set) {
		cout << entry << endl;
		if (recognized.find(entry.subject) == recognized.end()) {
			vertices[entry.subject] = boost::add_vertex(H);
			H[vertices[entry.subject]].name = entry.subject;
		}
		if (recognized.find(entry.object) == recognized.end()) {
			vertices[entry.object] = boost::add_vertex(H);
			H[vertices[entry.object]].name = entry.object;
		}
		boost::add_edge(vertices[entry.subject], vertices[entry.object], H);
		recognized.insert(entry.subject);
		recognized.insert(entry.object);
	}
	// printing the freshly constructed graph
	boost::print_graph(H, boost::get(&Vertex::name, H));
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
	std::cout << "Graph I:" << std::endl;
	boost::print_graph(I, boost::get(&Vertex::name, I));
	for (tie(ei_start, e_end) = boost::edges(I); ei_start != e_end;	++ei_start) {
//		auto source = boost::source(*ei_start, I);
//		auto target = boost::target(*ei_start, I);
		cout << boost::get(&Edge::edge_name, I, *ei_start) << endl;
	}
	hexastore::Hexastore hsi("hexastore_with_labels.sqlite");
	cout << "serialize I\n";
	for (tie(ei_start, e_end) = boost::edges(I); ei_start != e_end; ++ei_start) {
		auto source = boost::source(*ei_start, I);
		auto target = boost::target(*ei_start, I);
		string edge = boost::get(&Edge::edge_name, I, *ei_start);
		hsi.put( {{ I[source].name, edge, I[target].name }});
	}
	cout << "de-serialize I\n";
	graph_t J;
	set<string> recognized_vertices_hsi;
	map<string, vertex_t> vertices_hsi;
	vector<hexastore::hs_result> hsi_res = hsi.get({{"?", "?", "?"}}, 10);
	for (auto entry : hsi_res) {
		cout << entry << endl;
		if (recognized_vertices_hsi.find(entry.subject) == recognized_vertices_hsi.end()) {
			vertices_hsi[entry.subject] = boost::add_vertex(J);
			J[vertices_hsi[entry.subject]].name = entry.subject;
		}
		if (recognized_vertices_hsi.find(entry.object) == recognized_vertices_hsi.end()) {
			vertices_hsi[entry.object] = boost::add_vertex(J);
			J[vertices_hsi[entry.object]].name = entry.object;
		}
		recognized_vertices_hsi.insert(entry.subject);
		recognized_vertices_hsi.insert(entry.object);
		boost::add_edge(vertices_hsi[entry.subject], vertices_hsi[entry.object], Edge(entry.predicate), J);
	}
	// printing the freshly constructed graph
	boost::print_graph(J, boost::get(&Vertex::name, J));
	for (tie(ei_start, e_end) = boost::edges(J); ei_start != e_end; ++ei_start) {
		cout << boost::get(&Edge::edge_name, J, *ei_start) << endl;
	}
}
// Uncomment the code below to do some independent testing
// int main() {
// 	hs_more_tests();
//   cout << "\n";
//   hs_empty_fields_test();
// 	return 0;
// }