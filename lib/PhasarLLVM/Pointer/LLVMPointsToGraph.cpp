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
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "boost/graph/copy.hpp"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/graphviz.hpp"
#include "boost/log/sources/record_ostream.hpp"

#include "phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h"

#include "phasar/Utils/GraphExtensions.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Utilities.h"

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

std::vector<const llvm::User *>
PointsToGraph::VertexProperties::getUsers() const {
  if (!users.empty() || V == nullptr) {
    return users;
  }
  auto allUsers = V->users();
  users.insert(users.end(), allUsers.begin(), allUsers.end());
  return users;
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
  ContainedFunctions.insert(F);
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
  for (auto P : Pointers) {
    ValueVertexMap[P] = boost::add_vertex(VertexProperties(P), PAG);
  }
  // iterate over the worklist, and run the full (n^2)/2 disambiguations
  const auto mapEnd = ValueVertexMap.end();
  for (auto I1 = ValueVertexMap.begin(); I1 != mapEnd; ++I1) {
    llvm::Type *I1ElTy =
        llvm::cast<llvm::PointerType>(I1->first->getType())->getElementType();
    const uint64_t I1Size = I1ElTy->isSized()
                                ? DL.getTypeStoreSize(I1ElTy)
                                : llvm::MemoryLocation::UnknownSize;
    for (auto I2 = std::next(I1); I2 != mapEnd; ++I2) {
      llvm::Type *I2ElTy =
          llvm::cast<llvm::PointerType>(I2->first->getType())->getElementType();
      const uint64_t I2Size = I2ElTy->isSized()
                                  ? DL.getTypeStoreSize(I2ElTy)
                                  : llvm::MemoryLocation::UnknownSize;

      switch (AA.alias(I1->first, I1Size, I2->first, I2Size)) {
      case llvm::NoAlias:
        break;
      case llvm::MayAlias:     // no break
      case llvm::PartialAlias: // no break
      case llvm::MustAlias:
        boost::add_edge(I1->second, I2->second, PAG);
        break;
      default:
        break;
      }
    }
  }
}

vector<pair<unsigned, const llvm::Value *>>
PointsToGraph::getPointersEscapingThroughParams() {
  vector<pair<unsigned, const llvm::Value *>> escaping_pointers;
  for (auto vertexIter : boost::make_iterator_range(boost::vertices(PAG))) {
    if (const llvm::Argument *arg =
            llvm::dyn_cast<llvm::Argument>(PAG[vertexIter].V)) {
      escaping_pointers.push_back(make_pair(arg->getArgNo(), arg));
    }
  }
  return escaping_pointers;
}

vector<const llvm::Value *>
PointsToGraph::getPointersEscapingThroughReturns() const {
  vector<const llvm::Value *> escaping_pointers;
  for (auto vertexIter : boost::make_iterator_range(boost::vertices(PAG))) {
    auto &vertex = PAG[vertexIter];
    for (const auto user : vertex.getUsers()) {
      if (llvm::isa<llvm::ReturnInst>(user)) {
        escaping_pointers.push_back(vertex.V);
      }
    }
  }
  return escaping_pointers;
}

vector<const llvm::Value *>
PointsToGraph::getPointersEscapingThroughReturnsForFunction(
    const llvm::Function *F) const {
  vector<const llvm::Value *> escaping_pointers;
  for (auto vertexIter : boost::make_iterator_range(boost::vertices(PAG))) {
    auto &vertex = PAG[vertexIter];
    for (const auto user : vertex.getUsers()) {
      if (auto R = llvm::dyn_cast<llvm::ReturnInst>(user)) {
        if (R->getFunction() == F)
          escaping_pointers.push_back(vertex.V);
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
  for (auto vertexIter : boost::make_iterator_range(boost::vertices(PAG)))
    if (PAG[vertexIter].V == V)
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
    cout << "PointsToGraph for " << Fn->getName().str() << ":\n";
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

nlohmann::json PointsToGraph::getAsJson() const {
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

void PointsToGraph::mergeGraph(const PointsToGraph &Other) {
  typedef graph_t::vertex_descriptor vertex_t;
  typedef std::map<vertex_t, vertex_t> vertex_map_t;
  vertex_map_t oldToNewVertexMapping;
  boost::associative_property_map<vertex_map_t> vertexMapWrapper(
      oldToNewVertexMapping);
  boost::copy_graph(Other.PAG, PAG, boost::orig_to_copy(vertexMapWrapper));

  for (const auto &otherValues : Other.ValueVertexMap) {
    auto mappingIter = oldToNewVertexMapping.find(otherValues.second);
    if (mappingIter != oldToNewVertexMapping.end()) {
      ValueVertexMap.insert(make_pair(otherValues.first, mappingIter->second));
    }
  }
}

void PointsToGraph::mergeCallSite(const llvm::ImmutableCallSite &CS,
                                  const llvm::Function *F) {
  auto formalArgRange = F->args();
  auto formalIter = formalArgRange.begin();
  auto mapEnd = ValueVertexMap.end();
  for (const auto &arg : CS.args()) {
    const llvm::Argument *Formal = &*formalIter++;
    auto argMapIter = ValueVertexMap.find(arg);
    auto formalMapIter = ValueVertexMap.find(Formal);
    if (argMapIter != mapEnd && formalMapIter != mapEnd) {
      boost::add_edge(argMapIter->second, formalMapIter->second,
                      CS.getInstruction(), PAG);
    }
    if (formalIter == formalArgRange.end())
      break;
  }

  for (auto Formal : getPointersEscapingThroughReturnsForFunction(F)) {
    auto instrMapIter = ValueVertexMap.find(CS.getInstruction());
    auto formalMapIter = ValueVertexMap.find(Formal);
    if (instrMapIter != mapEnd && formalMapIter != mapEnd) {
      boost::add_edge(instrMapIter->second, formalMapIter->second,
                      CS.getInstruction(), PAG);
    }
  }
}

void PointsToGraph::mergeWith(const PointsToGraph *Other,
                              const llvm::Function *F) {
  if (ContainedFunctions.insert(F).second) {
    mergeGraph(*Other);
  }
}

void PointsToGraph::mergeWith(
    const PointsToGraph &Other,
    const vector<pair<llvm::ImmutableCallSite, const llvm::Function *>>
        &Calls) {
  ContainedFunctions.insert(Other.ContainedFunctions.begin(),
                            Other.ContainedFunctions.end());
  mergeGraph(Other);
  for (const auto &call : Calls) {
    mergeCallSite(call.first, call.second);
  }
}

bool PointsToGraph::empty() const { return size() == 0; }

size_t PointsToGraph::size() const { return getNumVertices(); }

size_t PointsToGraph::getNumVertices() const {
  return boost::num_vertices(PAG);
}

size_t PointsToGraph::getNumEdges() const { return boost::num_edges(PAG); }

void PointsToGraph::printAsJson(std::ostream &OS) const {
  nlohmann::json J = getAsJson();
  OS << J;
}

} // namespace psr
