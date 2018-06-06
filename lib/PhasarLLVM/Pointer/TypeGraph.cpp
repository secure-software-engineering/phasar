#include <phasar/PhasarLLVM/Pointer/TypeGraph.h>

using namespace std;

TypeGraph::dfs_visitor::dfs_visitor(TypeGraph::graph_t *_g) : g(_g) {}

void TypeGraph::dfs_visitor::finish_edge(TypeGraph::edge_t e, TypeGraph::graph_t const& u) {
  TypeGraph::vertex_t src = boost::source(e, u);
  TypeGraph::vertex_t target = boost::target(e, u);

  for (auto target_type : u[target].types) {
    auto name = uniformTypeName(target_type->getName().str());
    // (*g)[src].name += ", " + name; // To debug
    (*g)[src].types.insert(target_type);
  }
}

TypeGraph::reverse_type_propagation_dfs_visitor::reverse_type_propagation_dfs_visitor(rev_graph_t *_g) : g(_g) {}

void TypeGraph::reverse_type_propagation_dfs_visitor::examine_edge(rev_edge_t e, rev_graph_t const& u) {
  TypeGraph::rev_vertex_t src = boost::source(e, u);
  TypeGraph::rev_vertex_t target = boost::target(e, u);

  for (auto src_type : u[src].types) {
    auto name = uniformTypeName(src_type->getName().str());
    // (*g)[target].name += ", " + name; // To debug
    (*g)[target].types.insert(src_type);
  }
}

TypeGraph::vertex_t TypeGraph::addType(const llvm::StructType* new_type) {
  auto name = uniformTypeName(new_type->getName().str());

  if(type_vertex_map.find(name) == type_vertex_map.end()) {
    auto vertex = boost::add_vertex(g);
    type_vertex_map[name] = vertex;
    g[vertex].name = name;
    g[vertex].base_type = new_type;
    g[vertex].types.insert(new_type);
  }

  return type_vertex_map[name];
}

void TypeGraph::addLink(const llvm::StructType* from, const llvm::StructType* to) {
  if (already_visited)
    return;

  already_visited = true;

  auto from_vertex = addType(from);
  auto to_vertex = addType(to);

  boost::add_edge(from_vertex, to_vertex, g);
  reverseTypePropagation(to);

  for ( auto parent_g : parent_graphs ) {
    parent_g->addLink(from, to);
  }

  already_visited = false;
}

void TypeGraph::printAsDot(const std::string &path) const {
  std::ofstream ofs(path);
  boost::write_graphviz(
      ofs, g, boost::make_label_writer(boost::get(&TypeGraph::VertexProperties::name, g)));
}

void TypeGraph::aggregateTypes() {
  dfs_visitor vis(&g);
  boost::depth_first_search(g, boost::visitor(vis));
}

void TypeGraph::reverseTypePropagation(const llvm::StructType *base_struct) {
  auto name = uniformTypeName(base_struct->getName().str());

  std::vector<boost::default_color_type> color_map(boost::num_vertices(g));

  auto reversed = boost::reverse_graph<TypeGraph::graph_t, TypeGraph::graph_t&>(g);
  reverse_type_propagation_dfs_visitor vis(&reversed);

  boost::depth_first_visit(reversed,
    type_vertex_map[name],
    vis,
    boost::make_iterator_property_map(color_map.begin(), boost::get(boost::vertex_index, reversed), color_map[0]));
}

std::set<const llvm::StructType*> TypeGraph::getTypes(const llvm::StructType *struct_type) {
  auto struct_ty_vertex = addType(struct_type);

  return g[struct_ty_vertex].types;
}

void TypeGraph::merge(TypeGraph *tg) {
  tg->parent_graphs.insert(this);

  auto p = edges(tg->g);

  auto begin = p.first;
  auto end = p.second;

  for ( auto e = begin; e != end; ++e) {
    vertex_t src = boost::source(*e, tg->g);
    vertex_t target = boost::target(*e, tg->g);

    addLink(tg->g[src].base_type, tg->g[target].base_type);
  }
}
