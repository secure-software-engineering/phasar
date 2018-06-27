#pragma once

#include <map>
#include <set>
#include <string>

#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/graph/reverse_graph.hpp>

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Instruction.h>

#include <phasar/DB/ProjectIRDB.h>


namespace psr {
  class TypeGraph {
  public:
    struct VertexProperties {
      std::string name;
      std::set<const llvm::StructType*> types;
      const llvm::StructType* base_type;
    };

    struct EdgeProperties {
      EdgeProperties() = default;
    };

    typedef boost::adjacency_list<boost::setS, boost::vecS,
      boost::bidirectionalS, VertexProperties, EdgeProperties>
        graph_t;

    typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<graph_t>::vertex_iterator vertex_iterator;
    typedef boost::graph_traits<graph_t>::edge_descriptor edge_t;
    typedef boost::graph_traits<graph_t>::out_edge_iterator out_edge_iterator;
    typedef boost::graph_traits<graph_t>::in_edge_iterator in_edge_iterator;


    typedef boost::reverse_graph<graph_t, graph_t&> rev_graph_t;

    typedef boost::graph_traits<rev_graph_t>::vertex_descriptor rev_vertex_t;
    typedef boost::graph_traits<rev_graph_t>::vertex_iterator rev_vertex_iterator;
    typedef boost::graph_traits<rev_graph_t>::edge_descriptor rev_edge_t;
    typedef boost::graph_traits<rev_graph_t>::out_edge_iterator rev_out_edge_iterator;
    typedef boost::graph_traits<rev_graph_t>::in_edge_iterator rev_in_edge_iterator;

    struct dfs_visitor : public boost::default_dfs_visitor {
      dfs_visitor(graph_t *_g);

      void finish_edge(edge_t e, graph_t const& u);

      graph_t *g;
    };

    struct reverse_type_propagation_dfs_visitor : public boost::default_dfs_visitor {
      reverse_type_propagation_dfs_visitor(rev_graph_t *_g);

      void examine_edge(rev_edge_t e, rev_graph_t const& u);

      rev_graph_t *g;
    };

  private:
    std::map<std::string, vertex_t> type_vertex_map;
    std::set<TypeGraph*> parent_graphs;
    graph_t g;
    bool already_visited = false;

    FRIEND_TEST(TypeGraphTest, AddType);
    FRIEND_TEST(TypeGraphTest, AddLinkSimple);
    FRIEND_TEST(TypeGraphTest, AddLinkWithSubs);
    FRIEND_TEST(TypeGraphTest, AddLinkWithRecursion);
    FRIEND_TEST(TypeGraphTest, ReverseTypePropagation);
    FRIEND_TEST(TypeGraphTest, TypeAggregation);
    FRIEND_TEST(TypeGraphTest, Merging);

  public:
    TypeGraph() = default;

    virtual ~TypeGraph() = default;

    vertex_t addType(const llvm::StructType* new_type);
    void addLink(const llvm::StructType* from, const llvm::StructType* to);
    void addLinkWithoutReversePropagation(const llvm::StructType* from, const llvm::StructType* to);
    void printAsDot(const std::string &path = "typegraph.dot") const;
    void aggregateTypes();
    void reverseTypePropagation(const llvm::StructType *base_struct);
    std::set<const llvm::StructType*> getTypes(const llvm::StructType *struct_type);
    void merge(TypeGraph *tg);
  };
}
