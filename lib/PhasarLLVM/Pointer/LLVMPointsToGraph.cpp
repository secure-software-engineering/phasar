/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PointsToGraph.cpp
 *
 *  Created on: 08.02.2017
 *      Author: pdschbrt
 */
#include <llvm/ADT/SetVector.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h>

#include <phasar/Utils/GraphExtensions.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/PAMMMacros.h>
#include <phasar/Utils/Utilities.h>

using namespace std;
using namespace psr;

namespace psr {
struct PointsToGraph::AllocationSiteDFSVisitor : boost::default_dfs_visitor {
  // collect the allocation sites that are found
  std::set<const llvm::Value *> &allocation_sites;
  // keeps track of the current path
  std::vector<vertex_t> visitor_stack;
  // the call stack that can be matched against the visitor stack
  const std::vector<const llvm::Instruction *> &call_stack;

  AllocationSiteDFSVisitor(std::set<const llvm::Value *> &allocation_sizes,
                           const vector<const llvm::Instruction *> &call_stack)
      : allocation_sites(allocation_sizes), call_stack(call_stack) {}

  template <typename Vertex, typename Graph>
  void discover_vertex(Vertex u, const Graph &g) {
    visitor_stack.push_back(u);
  }

  template <typename Vertex, typename Graph>
  void finish_vertex(Vertex u, const Graph &g) {
    auto &lg = lg::get();
    // check for stack allocation
    if (const llvm::AllocaInst *Alloc =
            llvm::dyn_cast<llvm::AllocaInst>(g[u].V)) {
      // If the call stack is empty, we completely ignore the calling context
      if (matches_stack(g) || call_stack.empty()) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Found stack allocation: " << llvmIRToString(Alloc));
        allocation_sites.insert(g[u].V);
      }
    }
    // check for heap allocation
    if (llvm::isa<llvm::CallInst>(g[u].V) ||
        llvm::isa<llvm::InvokeInst>(g[u].V)) {
      llvm::ImmutableCallSite CS(g[u].V);
      if (CS.getCalledFunction() != nullptr &&
          HeapAllocationFunctions.count(
              CS.getCalledFunction()->getName().str())) {
        // If the call stack is empty, we completely ignore the calling
        // context
        if (matches_stack(g) || call_stack.empty()) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "Found heap allocation: "
                        << llvmIRToString(CS.getInstruction()));
          allocation_sites.insert(g[u].V);
        }
      }
    }
    visitor_stack.pop_back();
  }

  template <typename Graph> bool matches_stack(const Graph &g) {
    size_t call_stack_idx = 0;
    for (size_t i = 0, j = 1;
         i < visitor_stack.size() && j < visitor_stack.size(); ++i, ++j) {
      auto e = boost::edge(visitor_stack[i], visitor_stack[j], g);
      if (g[e.first].V == nullptr)
        continue;
      if (g[e.first].V != call_stack[call_stack.size() - call_stack_idx - 1]) {
        return false;
      }
      call_stack_idx++;
    }
    return true;
  }
};

struct PointsToGraph::ReachabilityDFSVisitor : boost::default_dfs_visitor {
  std::set<vertex_t> &points_to_set;
  ReachabilityDFSVisitor(set<vertex_t> &result) : points_to_set(result) {}
  template <typename Vertex, typename Graph>
  void finish_vertex(Vertex u, const Graph &g) {
    points_to_set.insert(u);
  }
};

// points-to graph internal stuff

PointsToGraph::VertexProperties::VertexProperties(const llvm::Value *V)
    : V(V) {}

std::string PointsToGraph::VertexProperties::getValueAsString() const {
  return llvmIRToString(V);
}

PointsToGraph::EdgeProperties::EdgeProperties(const llvm::Value *V) : V(V) {}

std::string PointsToGraph::EdgeProperties::getValueAsString() const {
  return llvmIRToString(V);
}

// points-to graph stuff

PointsToGraph::PointsToGraph(llvm::Function *F, llvm::AAResults &AA) {
  PAMM_GET_INSTANCE;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Analyzing function: " << F->getName().str());
  ContainedFunctions.insert(F->getName().str());
  bool PrintAll, PrintNoAlias, PrintMayAlias, PrintPartialAlias, PrintMustAlias,
      EvalAAMD, PrintNoModRef, PrintMod, PrintRef, PrintModRef, PrintMust,
      PrintMustMod, PrintMustRef, PrintMustModRef;
  PrintAll = PrintNoAlias = PrintMayAlias = PrintPartialAlias = PrintMustAlias =
      EvalAAMD = PrintNoModRef = PrintMod = PrintRef = PrintModRef = PrintMust =
          PrintMustMod = PrintMustRef = PrintMustModRef = true;

  // taken from llvm/Analysis/AliasAnalysisEvaluator.cpp
  const llvm::DataLayout &DL = F->getParent()->getDataLayout();

  llvm::SetVector<llvm::Value *> Pointers;
  llvm::SmallSetVector<llvm::CallBase *, 16> Calls;
  llvm::SetVector<llvm::Value *> Loads;
  llvm::SetVector<llvm::Value *> Stores;

  for (auto &I : F->args()) {
    if (I.getType()->isPointerTy()) { // Add all pointer arguments.
      Pointers.insert(&I);
    }
  }

  for (llvm::inst_iterator I = inst_begin(*F), E = inst_end(*F); I != E; ++I) {
    if (I->getType()->isPointerTy()) // Add all pointer instructions.
      Pointers.insert(&*I);
    if (EvalAAMD && llvm::isa<llvm::LoadInst>(&*I))
      Loads.insert(&*I);
    if (EvalAAMD && llvm::isa<llvm::StoreInst>(&*I))
      Stores.insert(&*I);
    llvm::Instruction &Inst = *I;
    if (auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst)) {
      llvm::Value *Callee = Call->getCalledValue();
      // Skip actual functions for direct function calls.
      if (!llvm::isa<llvm::Function>(Callee) && isInterestingPointer(Callee))
        Pointers.insert(Callee);
      // Consider formals.
      for (llvm::Use &DataOp : Call->data_ops())
        if (isInterestingPointer(DataOp))
          Pointers.insert(DataOp);
      Calls.insert(Call);
    } else {
      // Consider all operands.
      for (llvm::Instruction::op_iterator OI = Inst.op_begin(),
                                          OE = Inst.op_end();
           OI != OE; ++OI)
        if (isInterestingPointer(*OI))
          Pointers.insert(*OI);
    }
  }

  INC_COUNTER("GS Pointer", Pointers.size(), PAMM_SEVERITY_LEVEL::Core);

  // make vertices for all pointers
  for (auto Pointer : Pointers) {
    ValueVertexMap[Pointer] = boost::add_vertex(PAG);
    PAG[ValueVertexMap[Pointer]] = VertexProperties(Pointer);
  }
  // iterate over the worklist, and run the full (n^2)/2 disambiguations
  for (llvm::SetVector<llvm::Value *>::iterator I1 = Pointers.begin(),
                                                E = Pointers.end();
       I1 != E; ++I1) {
    uint64_t I1Size = llvm::MemoryLocation::UnknownSize;
    llvm::Type *I1ElTy =
        llvm::cast<llvm::PointerType>((*I1)->getType())->getElementType();
    if (I1ElTy->isSized()) {
      I1Size = DL.getTypeStoreSize(I1ElTy);
    }
    for (llvm::SetVector<llvm::Value *>::iterator I2 = Pointers.begin();
         I2 != I1; ++I2) {
      uint64_t I2Size = llvm::MemoryLocation::UnknownSize;
      llvm::Type *I2ElTy =
          llvm::cast<llvm::PointerType>((*I2)->getType())->getElementType();
      if (I2ElTy->isSized()) {
        I2Size = DL.getTypeStoreSize(I2ElTy);
      }
      switch (AA.alias(llvm::MemoryLocation(*I1, I1Size),
                       llvm::MemoryLocation(*I2, I2Size))) {
      case llvm::NoAlias:
        // NoAlias
        break;
      case llvm::MayAlias:
        boost::add_edge(ValueVertexMap[*I1], ValueVertexMap[*I2], PAG);
        break;
      case llvm::PartialAlias:
        boost::add_edge(ValueVertexMap[*I1], ValueVertexMap[*I2], PAG);
        break;
      case llvm::MustAlias:
        boost::add_edge(ValueVertexMap[*I1], ValueVertexMap[*I2], PAG);
        break;
      default:
        // do nothing
        break;
      }
    }
  }
}

vector<pair<unsigned, const llvm::Value *>>
PointsToGraph::getPointersEscapingThroughParams() {
  vector<pair<unsigned, const llvm::Value *>> escaping_pointers;
  for (pair<vertex_iterator, vertex_iterator> vp = boost::vertices(PAG);
       vp.first != vp.second; ++vp.first) {
    if (const llvm::Argument *arg =
            llvm::dyn_cast<llvm::Argument>(PAG[*vp.first].V)) {
      escaping_pointers.push_back(make_pair(arg->getArgNo(), arg));
    }
  }
  return escaping_pointers;
}

vector<const llvm::Value *>
PointsToGraph::getPointersEscapingThroughReturns() const {
  vector<const llvm::Value *> escaping_pointers;
  for (pair<vertex_iterator, vertex_iterator> vp = boost::vertices(PAG);
       vp.first != vp.second; ++vp.first) {
    for (auto user : PAG[*vp.first].V->users()) {
      if (llvm::isa<llvm::ReturnInst>(user)) {
        escaping_pointers.push_back(PAG[*vp.first].V);
      }
    }
  }
  return escaping_pointers;
}

vector<const llvm::Value *>
PointsToGraph::getPointersEscapingThroughReturnsForFunction(
    const llvm::Function *F) const {
  vector<const llvm::Value *> escaping_pointers;
  for (pair<vertex_iterator, vertex_iterator> vp = boost::vertices(PAG);
       vp.first != vp.second; ++vp.first) {
    for (auto user : PAG[*vp.first].V->users()) {
      if (auto R = llvm::dyn_cast<llvm::ReturnInst>(user)) {
        if (R->getFunction() == F)
          escaping_pointers.push_back(PAG[*vp.first].V);
      }
    }
  }
  return escaping_pointers;
}

set<const llvm::Value *> PointsToGraph::getReachableAllocationSites(
    const llvm::Value *V, vector<const llvm::Instruction *> CallStack) {
  set<const llvm::Value *> alloc_sites;
  AllocationSiteDFSVisitor alloc_vis(alloc_sites, CallStack);
  vector<boost::default_color_type> color_map(boost::num_vertices(PAG));
  boost::depth_first_visit(
      PAG, ValueVertexMap[V], alloc_vis,
      boost::make_iterator_property_map(color_map.begin(),
                                        boost::get(boost::vertex_index, PAG),
                                        color_map[0]));
  return alloc_sites;
}

bool PointsToGraph::containsValue(llvm::Value *V) {
  pair<vertex_iterator, vertex_iterator> vp;
  for (vp = boost::vertices(PAG); vp.first != vp.second; ++vp.first)
    if (PAG[*vp.first].V == V)
      return true;
  return false;
}

set<const llvm::Type *>
PointsToGraph::computeTypesFromAllocationSites(set<const llvm::Value *> AS) {
  set<const llvm::Type *> types;
  // an allocation site can either be an AllocaInst or a call to an allocating
  // function
  for (auto V : AS) {
    if (const llvm::AllocaInst *alloc = llvm::dyn_cast<llvm::AllocaInst>(V)) {
      types.insert(alloc->getAllocatedType());
    } else {
      // usually if an allocating function is called, it is immediately
      // bit-casted
      // to the desired allocated value and hence we can determine it from the
      // destination type of that cast instruction.
      for (auto user : V->users()) {
        if (const llvm::BitCastInst *cast =
                llvm::dyn_cast<llvm::BitCastInst>(user)) {
          types.insert(cast->getDestTy());
        }
      }
    }
  }
  return types;
}

set<const llvm::Value *>
PointsToGraph::getPointsToSet(const llvm::Value *V) const {
  PAMM_GET_INSTANCE;
  INC_COUNTER("[Calls] getPointsToSet", 1, PAMM_SEVERITY_LEVEL::Full);
  START_TIMER("PointsTo-Set Computation", PAMM_SEVERITY_LEVEL::Full);
  // check if the graph contains a corresponding vertex
  if (!ValueVertexMap.count(V)) {
    return {};
  }
  set<vertex_t> reachable_vertices;
  ReachabilityDFSVisitor vis(reachable_vertices);
  vector<boost::default_color_type> color_map(boost::num_vertices(PAG));
  boost::depth_first_visit(
      PAG, ValueVertexMap.at(V), vis,
      boost::make_iterator_property_map(color_map.begin(),
                                        boost::get(boost::vertex_index, PAG),
                                        color_map[0]));
  set<const llvm::Value *> result;
  for (auto vertex : reachable_vertices) {
    result.insert(PAG[vertex].V);
  }
  PAUSE_TIMER("PointsTo-Set Computation", PAMM_SEVERITY_LEVEL::Full);
  ADD_TO_HISTOGRAM("Points-to", result.size(), 1, PAMM_SEVERITY_LEVEL::Full);
  return result;
}

bool PointsToGraph::representsSingleFunction() {
  return ContainedFunctions.size() == 1;
}

void PointsToGraph::print(std::ostream &OS) const {
  for (const auto &Fn : ContainedFunctions) {
    cout << "PointsToGraph for " << Fn << ":\n";
    vertex_iterator ui, ui_end;
    for (boost::tie(ui, ui_end) = boost::vertices(PAG); ui != ui_end; ++ui) {
      OS << PAG[*ui].getValueAsString() << " <--> ";
      out_edge_iterator ei, ei_end;
      for (boost::tie(ei, ei_end) = boost::out_edges(*ui, PAG); ei != ei_end;
           ++ei) {
        OS << PAG[target(*ei, PAG)].getValueAsString() << " ";
      }
      OS << '\n';
    }
  }
}

void PointsToGraph::printAsDot(std::ostream &OS) const {
  boost::write_graphviz(OS, PAG, makePointerVertexOrEdgePrinter(PAG),
                        makePointerVertexOrEdgePrinter(PAG));
}

nlohmann::json PointsToGraph::getAsJson() {
  nlohmann::json J;
  vertex_iterator vi_v, vi_v_end;
  out_edge_iterator ei, ei_end;
  // iterate all graph vertices
  for (boost::tie(vi_v, vi_v_end) = boost::vertices(PAG); vi_v != vi_v_end;
       ++vi_v) {
    J[PhasarConfig::JsonPointToGraphID()][PAG[*vi_v].getValueAsString()];
    // iterate all out edges of vertex vi_v
    for (boost::tie(ei, ei_end) = boost::out_edges(*vi_v, PAG); ei != ei_end;
         ++ei) {
      J[PhasarConfig::JsonPointToGraphID()][PAG[*vi_v].getValueAsString()] +=
          PAG[boost::target(*ei, PAG)].getValueAsString();
    }
  }
  return J;
}

void PointsToGraph::printValueVertexMap() {
  for (const auto &entry : ValueVertexMap) {
    cout << entry.first << " <---> " << entry.second << endl;
  }
}

void PointsToGraph::mergeWith(const PointsToGraph *Other,
                              const llvm::Function *F) {
  if (!ContainedFunctions.count(F->getName().str())) {
    ContainedFunctions.insert(F->getName().str());
    copy_graph<PointsToGraph::graph_t, PointsToGraph::vertex_t>(PAG,
                                                                Other->PAG);
    ValueVertexMap.clear();
    vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = boost::vertices(PAG); vi != vi_end; ++vi) {
      ValueVertexMap.insert(make_pair(PAG[*vi].V, *vi));
    }
  }
}

void PointsToGraph::mergeWith(
    const PointsToGraph &Other,
    const vector<pair<llvm::ImmutableCallSite, const llvm::Function *>>
        &Calls) {
  vector<tuple<PointsToGraph::vertex_t, PointsToGraph::vertex_t,
               const llvm::Instruction *>>
      v_in_g1_u_in_g2;
  for (auto Call : Calls) {
    cout << "performing parameter mapping\n";
    for (unsigned i = 0; i < Call.first.getNumArgOperands(); ++i) {
      auto Formal = getNthFunctionArgument(Call.second, i);
      // cout << "CONTAINS VALUE IN PARAMLIST: " <<
      // Other.ValueVertexMap.count(Formal) << endl;
      // Check if the value is of type pointer, therefore it must be contained
      // in the value_vertex_maps
      if (ValueVertexMap.count(Call.first.getArgOperand(i)) &&
          Other.ValueVertexMap.count(Formal)) {
        v_in_g1_u_in_g2.push_back(
            tuple<PointsToGraph::vertex_t, PointsToGraph::vertex_t,
                  const llvm::Instruction *>(
                ValueVertexMap[Call.first.getArgOperand(i)],
                Other.ValueVertexMap.at(Formal), Call.first.getInstruction()));
      }
    }

    for (auto Formal :
         Other.getPointersEscapingThroughReturnsForFunction(Call.second)) {
      if (ValueVertexMap.count(Call.first.getInstruction()) &&
          Other.ValueVertexMap.count(Formal)) {
        v_in_g1_u_in_g2.push_back(
            tuple<PointsToGraph::vertex_t, PointsToGraph::vertex_t,
                  const llvm::Instruction *>(
                ValueVertexMap[Call.first.getInstruction()],
                Other.ValueVertexMap.at(Formal), Call.first.getInstruction()));
      }
    }
    ContainedFunctions.insert(Call.second->getName().str());
  }
  merge_graphs<PointsToGraph::graph_t, PointsToGraph::vertex_t,
               PointsToGraph::EdgeProperties, const llvm::Instruction *>(
      PAG, Other.PAG, v_in_g1_u_in_g2);
  ValueVertexMap.clear();
  vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = boost::vertices(PAG); vi != vi_end; ++vi) {
    ValueVertexMap.insert(make_pair(PAG[*vi].V, *vi));
  }
}

void PointsToGraph::mergeWith(PointsToGraph *Other, llvm::ImmutableCallSite CS,
                              const llvm::Function *F) {
  // Check if points-to graph of F is already within 'this' whole module
  // points-to graph
  if (ContainedFunctions.count(F->getName().str())) {
    for (unsigned i = 0; i < CS.getNumArgOperands(); ++i) {
      auto Formal = getNthFunctionArgument(F, i);
      // Only draw the edges, when these values are of type pointer and
      // therefore contained in ValueVertexMap
      if (ValueVertexMap.count(CS.getArgOperand(i)) &&
          ValueVertexMap.count(Formal)) {
        boost::add_edge(ValueVertexMap[CS.getArgOperand(i)],
                        ValueVertexMap[Formal], CS.getInstruction(), PAG);
      }
    }

    for (auto Formal : getPointersEscapingThroughReturnsForFunction(F)) {
      if (ValueVertexMap.count(CS.getInstruction()) &&
          ValueVertexMap.count(Formal)) {
        boost::add_edge(ValueVertexMap[CS.getInstruction()],
                        ValueVertexMap[Formal], CS.getInstruction(), PAG);
      }
    }
  } else {
    ContainedFunctions.insert(F->getName().str());
    // TODO this function has to check if F's points-to graph is already
    // merged into the 'this' points-to graph. If so, is is not allowed to
    // copy it a second time into 'this' PAG.
    vector<pair<PointsToGraph::vertex_t, PointsToGraph::vertex_t>>
        v_in_g1_u_in_g2;
    for (unsigned i = 0; i < CS.getNumArgOperands(); ++i) {
      auto Formal = getNthFunctionArgument(F, i);
      if (ValueVertexMap.count(CS.getArgOperand(i)) &&
          Other->ValueVertexMap.count(Formal)) {
        v_in_g1_u_in_g2.push_back(make_pair(ValueVertexMap[CS.getArgOperand(i)],
                                            Other->ValueVertexMap[Formal]));
      }
    }

    for (auto Formal : Other->getPointersEscapingThroughReturnsForFunction(F)) {
      if (ValueVertexMap.count(CS.getInstruction()) &&
          Other->ValueVertexMap.count(Formal)) {
        v_in_g1_u_in_g2.push_back(make_pair(ValueVertexMap[CS.getInstruction()],
                                            Other->ValueVertexMap[Formal]));
      }
    }

    typedef
        typename boost::property_map<PointsToGraph::graph_t,
                                     boost::vertex_index_t>::type index_map_t;
    // for simple adjacency_list<> this type would be more efficient:
    typedef typename boost::iterator_property_map<
        typename std::vector<PointsToGraph::vertex_t>::iterator, index_map_t,
        PointsToGraph::vertex_t, PointsToGraph::vertex_t &>
        IsoMap;
    // for more generic graphs, one can try typedef std::map<vertex_t,
    // vertex_t> IsoMap;
    vector<PointsToGraph::vertex_t> orig2copy_data(
        boost::num_vertices(Other->PAG));
    IsoMap mapV = boost::make_iterator_property_map(
        orig2copy_data.begin(), get(boost::vertex_index, Other->PAG));
    boost::copy_graph(Other->PAG, PAG,
                      boost::orig_to_copy(mapV)); // means g1 += g2
    for (auto &entry : v_in_g1_u_in_g2) {
      PointsToGraph::vertex_t u_in_g1 = mapV[entry.second];
      boost::add_edge(entry.first, u_in_g1, CS.getInstruction(), PAG);
    }
  }
  ValueVertexMap.clear();
  vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = boost::vertices(PAG); vi != vi_end; ++vi) {
    ValueVertexMap.insert(make_pair(PAG[*vi].V, *vi));
  }
}

bool PointsToGraph::empty() const { return size() == 0; }

size_t PointsToGraph::size() const { return getNumVertices(); }

size_t PointsToGraph::getNumVertices() const {
  return boost::num_vertices(PAG);
}

size_t PointsToGraph::getNumEdges() const { return boost::num_edges(PAG); }

} // namespace psr
