/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "boost/graph/copy.hpp"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/graphviz.hpp"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasedPointsToAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/Utils/GraphExtensions.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

namespace psr {
struct LLVMPointsToGraph::AllocationSiteDFSVisitor
    : boost::default_dfs_visitor {
  // collect the allocation sites that are found
  PointsToSetTy &AllocationSites;
  // keeps track of the current path
  std::vector<vertex_t> VisitorStack;
  // the call stack that can be matched against the visitor stack
  const std::vector<const llvm::Instruction *> &CallStack;

  AllocationSiteDFSVisitor(PointsToSetTy &AllocationSizes,
                           const vector<const llvm::Instruction *> &CallStack)
      : AllocationSites(AllocationSizes), CallStack(CallStack) {}

  template <typename Vertex, typename Graph>
  void discover_vertex(Vertex U, const Graph & /*G*/) {
    VisitorStack.push_back(U);
  }

  template <typename Vertex, typename Graph>
  void finish_vertex(Vertex U, const Graph &G) {
    // check for stack allocation
    if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(G[U].V)) {
      // If the call stack is empty, we completely ignore the calling context
      if (matchesStack(G) || CallStack.empty()) {
        PHASAR_LOG_LEVEL(DEBUG,
                         "Found stack allocation: " << llvmIRToString(Alloc));
        AllocationSites.insert(G[U].V);
      }
    }
    // check for heap allocation
    if (llvm::isa<llvm::CallInst>(G[U].V) ||
        llvm::isa<llvm::InvokeInst>(G[U].V)) {
      const auto *CallSite = llvm::cast<llvm::CallBase>(G[U].V);
      if (CallSite->getCalledFunction() != nullptr &&
          HeapAllocatingFunctions.count(
              CallSite->getCalledFunction()->getName())) {
        // If the call stack is empty, we completely ignore the calling
        // context
        if (matchesStack(G) || CallStack.empty()) {
          PHASAR_LOG_LEVEL(
              DEBUG, "Found heap allocation: " << llvmIRToString(CallSite));
          AllocationSites.insert(G[U].V);
        }
      }
    }
    VisitorStack.pop_back();
  }

  template <typename Graph> bool matchesStack(const Graph &G) {
    size_t CallStackIdx = 0;
    for (size_t I = 0, J = 1;
         I < VisitorStack.size() && J < VisitorStack.size(); ++I, ++J) {
      auto E = boost::edge(VisitorStack[I], VisitorStack[J], G);
      if (G[E.first].V == nullptr) {
        continue;
      }
      if (G[E.first].V != CallStack[CallStack.size() - CallStackIdx - 1]) {
        return false;
      }
      CallStackIdx++;
    }
    return true;
  }
};

struct LLVMPointsToGraph::ReachabilityDFSVisitor : boost::default_dfs_visitor {
  std::set<vertex_t> &PointsToSet;
  ReachabilityDFSVisitor(set<vertex_t> &Result) : PointsToSet(Result) {}
  template <typename Vertex, typename Graph>
  void finish_vertex(Vertex U, const Graph & /*Graph*/) {
    PointsToSet.insert(U);
  }
};

// points-to graph internal stuff

LLVMPointsToGraph::VertexProperties::VertexProperties(const llvm::Value *V)
    : V(V) {}

std::string LLVMPointsToGraph::VertexProperties::getValueAsString() const {
  return llvmIRToString(V);
}

std::vector<const llvm::User *>
LLVMPointsToGraph::VertexProperties::getUsers() const {
  if (!Users.empty() || V == nullptr) {
    return Users;
  }
  auto AllUsers = V->users();
  Users.insert(Users.end(), AllUsers.begin(), AllUsers.end());
  return Users;
}

LLVMPointsToGraph::EdgeProperties::EdgeProperties(const llvm::Value *V)
    : V(V) {}

std::string LLVMPointsToGraph::EdgeProperties::getValueAsString() const {
  return llvmIRToString(V);
}

// points-to graph stuff

LLVMPointsToGraph::LLVMPointsToGraph(ProjectIRDB &IRDB, bool UseLazyEvaluation,
                                     PointerAnalysisType PATy)
    : PTA(IRDB, UseLazyEvaluation, PATy) {}

void LLVMPointsToGraph::computePointsToGraph(const llvm::Value *V) {
  // FIXME when fixed in LLVM
  auto *VF = const_cast<llvm::Function *>(retrieveFunction(V)); // NOLINT
  computePointsToGraph(VF);
}

void LLVMPointsToGraph::computePointsToGraph(llvm::Function *F) {
  // check if we already analyzed the function
  if (AnalyzedFunctions.find(F) != AnalyzedFunctions.end()) {
    return;
  }
  PAMM_GET_INSTANCE;
  PHASAR_LOG_LEVEL(DEBUG, "Analyzing function: " << F->getName());
  AnalyzedFunctions.insert(F);
  llvm::AAResults &AA = *PTA.getAAResults(F);
  bool EvalAAMD = true;

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
    if (I->getType()->isPointerTy()) { // Add all pointer instructions.
      Pointers.insert(&*I);
    }
    if (EvalAAMD && llvm::isa<llvm::LoadInst>(&*I)) {
      Loads.insert(&*I);
    }
    if (EvalAAMD && llvm::isa<llvm::StoreInst>(&*I)) {
      Stores.insert(&*I);
    }
    llvm::Instruction &Inst = *I;
    if (auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst)) {
      llvm::Value *Callee = Call->getCalledOperand();
      // Skip actual functions for direct function calls.
      if (!llvm::isa<llvm::Function>(Callee) && isInterestingPointer(Callee)) {
        Pointers.insert(Callee);
      }
      // Consider formals.
      for (llvm::Use &DataOp : Call->data_ops()) {
        if (isInterestingPointer(DataOp)) {
          Pointers.insert(DataOp);
        }
      }
      Calls.insert(Call);
    } else {
      // Consider all operands.
      for (llvm::Instruction::op_iterator OI = Inst.op_begin(),
                                          OE = Inst.op_end();
           OI != OE; ++OI) {
        if (isInterestingPointer(*OI)) {
          Pointers.insert(*OI);
        }
      }
    }
  }

  INC_COUNTER("GS Pointer", Pointers.size(), PAMM_SEVERITY_LEVEL::Core);

  // make vertices for all pointers
  for (auto *P : Pointers) {
    ValueVertexMap[P] = boost::add_vertex(VertexProperties(P), PAG);
  }
  // iterate over the worklist, and run the full (n^2)/2 disambiguations
  const auto MapEnd = ValueVertexMap.end();
  for (auto I1 = ValueVertexMap.begin(); I1 != MapEnd; ++I1) {
    llvm::Type *I1ElTy =
        llvm::cast<llvm::PointerType>(I1->first->getType())->getElementType();
    const uint64_t I1Size = I1ElTy->isSized()
                                ? DL.getTypeStoreSize(I1ElTy)
                                : llvm::MemoryLocation::UnknownSize;
    for (auto I2 = std::next(I1); I2 != MapEnd; ++I2) {
      llvm::Type *I2ElTy =
          llvm::cast<llvm::PointerType>(I2->first->getType())->getElementType();
      const uint64_t I2Size = I2ElTy->isSized()
                                  ? DL.getTypeStoreSize(I2ElTy)
                                  : llvm::MemoryLocation::UnknownSize;
      switch (AA.alias(I1->first, I1Size, I2->first, I2Size)) {
      case llvm::AliasResult::NoAlias:
        break;
      case llvm::AliasResult::MayAlias: // no break
        [[fallthrough]];
      case llvm::AliasResult::PartialAlias: // no break
        [[fallthrough]];
      case llvm::AliasResult::MustAlias:
        boost::add_edge(I1->second, I2->second, PAG);
        break;
      default:
        break;
      }
    }
  }
}

bool LLVMPointsToGraph::isInterProcedural() const { return false; }

PointerAnalysisType LLVMPointsToGraph::getPointerAnalysistype() const {
  return PTA.getPointerAnalysisType();
}

AliasResult LLVMPointsToGraph::alias(const llvm::Value *V1,
                                     const llvm::Value *V2,
                                     const llvm::Instruction * /*I*/) {
  computePointsToGraph(V1);
  computePointsToGraph(V2);
  auto PTS = getPointsToSet(V1);
  if (PTS->contains(V2)) {
    return AliasResult::MustAlias;
  }
  return AliasResult::NoAlias;
}

auto LLVMPointsToGraph::getReachableAllocationSites(
    const llvm::Value *V, bool /*IntraProcOnly*/,
    const llvm::Instruction * /*I*/) -> AllocationSiteSetPtrTy {
  computePointsToGraph(V);
  auto AllocSites = std::make_unique<PointsToSetTy>();
  AllocationSiteDFSVisitor AllocVis(*AllocSites, {});
  vector<boost::default_color_type> ColorMap(boost::num_vertices(PAG));
  boost::depth_first_visit(
      PAG, ValueVertexMap[V], AllocVis,
      boost::make_iterator_property_map(
          ColorMap.begin(), boost::get(boost::vertex_index, PAG), ColorMap[0]));
  return AllocSites;
}

[[nodiscard]] bool LLVMPointsToGraph::isInReachableAllocationSites(
    const llvm::Value *V, const llvm::Value *PotentialValue, bool IntraProcOnly,
    const llvm::Instruction *I) {
  return getReachableAllocationSites(V, IntraProcOnly, I)
      ->count(PotentialValue);
}

void LLVMPointsToGraph::mergeWith(
    const LLVMPointsToGraph::PointsToInfo<const llvm::Value *,
                                          const llvm::Instruction *> &PTI) {
  const auto *OtherPTI = dynamic_cast<const LLVMPointsToGraph *>(&PTI);
  if (!OtherPTI) {
    llvm::report_fatal_error(
        "LLVMPointsToSet can only be merged with another LLVMPointsToSet!");
  }
  AnalyzedFunctions.insert(OtherPTI->AnalyzedFunctions.begin(),
                           OtherPTI->AnalyzedFunctions.end());
  using vertex_t = graph_t::vertex_descriptor;
  using vertex_map_t = std::map<vertex_t, vertex_t>;
  vertex_map_t OldToNewVertexMapping;
  boost::associative_property_map<vertex_map_t> VertexMapWrapper(
      OldToNewVertexMapping);
  boost::copy_graph(OtherPTI->PAG, PAG, boost::orig_to_copy(VertexMapWrapper));
  for (const auto &OtherValues : OtherPTI->ValueVertexMap) {
    auto Search = OldToNewVertexMapping.find(OtherValues.second);
    if (Search != OldToNewVertexMapping.end()) {
      ValueVertexMap.insert(make_pair(OtherValues.first, Search->second));
    }
  }
}

void LLVMPointsToGraph::introduceAlias(const llvm::Value *V1,
                                       const llvm::Value *V2,
                                       const llvm::Instruction *I,
                                       AliasResult /*Kind*/) {
  computePointsToGraph(V1);
  computePointsToGraph(V2);
  auto Vert1 = ValueVertexMap[V1];
  auto Vert2 = ValueVertexMap[V2];
  boost::add_edge(Vert1, Vert2, I, PAG);
}

vector<pair<unsigned, const llvm::Value *>>
LLVMPointsToGraph::getPointersEscapingThroughParams() {
  vector<pair<unsigned, const llvm::Value *>> EscapingPointers;
  for (auto VertexIter : boost::make_iterator_range(boost::vertices(PAG))) {
    if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(PAG[VertexIter].V)) {
      EscapingPointers.emplace_back(Arg->getArgNo(), Arg);
    }
  }
  return EscapingPointers;
}

vector<const llvm::Value *>
LLVMPointsToGraph::getPointersEscapingThroughReturns() const {
  vector<const llvm::Value *> EscapingPointers;
  for (auto VertexIter : boost::make_iterator_range(boost::vertices(PAG))) {
    const auto &Vertex = PAG[VertexIter];
    for (const auto *const User : Vertex.getUsers()) {
      if (llvm::isa<llvm::ReturnInst>(User)) {
        EscapingPointers.push_back(Vertex.V);
      }
    }
  }
  return EscapingPointers;
}

vector<const llvm::Value *>
LLVMPointsToGraph::getPointersEscapingThroughReturnsForFunction(
    const llvm::Function *F) const {
  vector<const llvm::Value *> EscapingPointers;
  for (auto VertexIter : boost::make_iterator_range(boost::vertices(PAG))) {
    const auto &Vertex = PAG[VertexIter];
    for (const auto *const User : Vertex.getUsers()) {
      if (const auto *R = llvm::dyn_cast<llvm::ReturnInst>(User)) {
        if (R->getFunction() == F) {
          EscapingPointers.push_back(Vertex.V);
        }
      }
    }
  }
  return EscapingPointers;
}

bool LLVMPointsToGraph::containsValue(llvm::Value *V) {
  for (auto VertexIter : boost::make_iterator_range(boost::vertices(PAG))) {
    if (PAG[VertexIter].V == V) {
      return true;
    }
  }
  return false;
}

auto LLVMPointsToGraph::getPointsToSet(const llvm::Value *V,
                                       const llvm::Instruction * /*I*/)
    -> PointsToSetPtrTy {
  PAMM_GET_INSTANCE;
  INC_COUNTER("[Calls] getPointsToSet", 1, PAMM_SEVERITY_LEVEL::Full);
  START_TIMER("PointsTo-Set Computation", PAMM_SEVERITY_LEVEL::Full);
  const auto *VF = retrieveFunction(V);
  computePointsToGraph(VF);
  // check if the graph contains a corresponding vertex
  set<vertex_t> ReachableVertices;
  ReachabilityDFSVisitor Vis(ReachableVertices);
  vector<boost::default_color_type> ColorMap(boost::num_vertices(PAG));
  boost::depth_first_visit(
      PAG, ValueVertexMap.at(V), Vis,
      boost::make_iterator_property_map(
          ColorMap.begin(), boost::get(boost::vertex_index, PAG), ColorMap[0]));
  auto ResultSet = [this, V] {
    auto &Ret = Cache[V];

    if (!Ret) {
      Ret = Owner.acquire();
    }

    return Ret;
  }();

  for (auto Vertex : ReachableVertices) {
    ResultSet->insert(PAG[Vertex].V);
  }
  PAUSE_TIMER("PointsTo-Set Computation", PAMM_SEVERITY_LEVEL::Full);
  ADD_TO_HISTOGRAM("Points-to", ResultSet->size(), 1,
                   PAMM_SEVERITY_LEVEL::Full);
  return ResultSet;
}

void LLVMPointsToGraph::print(llvm::raw_ostream &OS) const {
  for (const auto &Fn : AnalyzedFunctions) {
    llvm::outs() << "LLVMPointsToGraph for " << Fn->getName() << ":\n";
    vertex_iterator UI;

    vertex_iterator UIEnd;
    for (boost::tie(UI, UIEnd) = boost::vertices(PAG); UI != UIEnd; ++UI) {
      OS << PAG[*UI].getValueAsString() << " <--> ";
      out_edge_iterator EI;

      out_edge_iterator EIEnd;
      for (boost::tie(EI, EIEnd) = boost::out_edges(*UI, PAG); EI != EIEnd;
           ++EI) {
        OS << PAG[target(*EI, PAG)].getValueAsString() << " ";
      }
      OS << '\n';
    }
  }
}

void LLVMPointsToGraph::printAsDot(llvm::raw_ostream &OS) const {
  std::stringstream S;
  boost::write_graphviz(S, PAG, makePointerVertexOrEdgePrinter(PAG),
                        makePointerVertexOrEdgePrinter(PAG));
  OS << S.str();
}

nlohmann::json LLVMPointsToGraph::getAsJson() const {
  nlohmann::json J;
  vertex_iterator VIv;

  vertex_iterator VIvEnd;
  out_edge_iterator EI;

  out_edge_iterator EIEnd;
  // iterate all graph vertices
  for (boost::tie(VIv, VIvEnd) = boost::vertices(PAG); VIv != VIvEnd; ++VIv) {
    J[PhasarConfig::JsonPointsToGraphID()][PAG[*VIv].getValueAsString()];
    // iterate all out edges of vertex vi_v
    for (boost::tie(EI, EIEnd) = boost::out_edges(*VIv, PAG); EI != EIEnd;
         ++EI) {
      J[PhasarConfig::JsonPointsToGraphID()][PAG[*VIv].getValueAsString()] +=
          PAG[boost::target(*EI, PAG)].getValueAsString();
    }
  }
  return J;
}

void LLVMPointsToGraph::printValueVertexMap() {
  for (const auto &Entry : ValueVertexMap) {
    llvm::outs() << Entry.first << " <---> " << Entry.second << '\n';
  }
}

bool LLVMPointsToGraph::empty() const { return size() == 0; }

size_t LLVMPointsToGraph::size() const { return getNumVertices(); }

size_t LLVMPointsToGraph::getNumVertices() const {
  return boost::num_vertices(PAG);
}

size_t LLVMPointsToGraph::getNumEdges() const { return boost::num_edges(PAG); }

void LLVMPointsToGraph::printAsJson(llvm::raw_ostream &OS) const {
  nlohmann::json J = getAsJson();
  OS << J;
}

} // namespace psr
