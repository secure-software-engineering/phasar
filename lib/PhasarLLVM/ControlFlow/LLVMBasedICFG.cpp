/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMBasedInterproceduralICFG.cpp
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#include <cassert>
#include <initializer_list>
#include <memory>
#include <ostream>

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "boost/graph/copy.hpp"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/graphviz.hpp"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/DTAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/NOResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/OTFResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/PhasarPass/Options.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Utilities.h"

#include "nlohmann/json.hpp"

using namespace psr;
using namespace std;

// Define some handy helper functionalities
namespace {
template <class graphType> class VertexWriter {
public:
  VertexWriter(const graphType &CGraph) : CGraph(CGraph) {}
  template <class VertexOrEdge>
  void operator()(std::ostream &Out, const VertexOrEdge &V) const {
    Out << "[label=\"" << CGraph[V].getFunctionName() << "\"]";
  }

private:
  const graphType &CGraph;
};

template <class graphType> class EdgeLabelWriter {
public:
  EdgeLabelWriter(const graphType &CGraph) : CGraph(CGraph) {}
  template <class VertexOrEdge>
  void operator()(std::ostream &Out, const VertexOrEdge &V) const {
    Out << "[label=\"" << CGraph[V].getCallSiteAsString() << "\"]";
  }

private:
  const graphType &CGraph;
};

} // anonymous namespace

namespace psr {

struct LLVMBasedICFG::dependency_visitor : boost::default_dfs_visitor {
  std::vector<vertex_t> &Vertices;
  dependency_visitor(std::vector<vertex_t> &V) : Vertices(V) {}
  template <typename Vertex, typename Graph>
  void finish_vertex(Vertex U, const Graph &G) {
    Vertices.push_back(U);
  }
};

LLVMBasedICFG::VertexProperties::VertexProperties(const llvm::Function *F)
    : F(F) {}

std::string LLVMBasedICFG::VertexProperties::getFunctionName() const {
  return F->getName().str();
}

LLVMBasedICFG::EdgeProperties::EdgeProperties(const llvm::Instruction *I)
    : CS(I), ID(stoull(getMetaDataID(I))) {}

std::string LLVMBasedICFG::EdgeProperties::getCallSiteAsString() const {
  return llvmIRToString(CS);
}

// Need to provide copy constructor explicitly to avoid multiple frees of TH and
// PT in case any of them is allocated within the constructor. To this end, we
// set UserTHInfos and UserPTInfos to true here.
LLVMBasedICFG::LLVMBasedICFG(const LLVMBasedICFG &ICF)
    : IRDB(ICF.IRDB), CGType(ICF.CGType), S(ICF.S), TH(ICF.TH), PT(ICF.PT),
      // TODO copy resolver
      Res(nullptr), VisitedFunctions(ICF.VisitedFunctions),
      CallGraph(ICF.CallGraph), FunctionVertexMap(ICF.FunctionVertexMap) {}

LLVMBasedICFG::LLVMBasedICFG(ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
                             const std::set<std::string> &EntryPoints,
                             LLVMTypeHierarchy *TH, LLVMPointsToInfo *PT,
                             Soundness S, bool IncludeGlobals)
    : IRDB(IRDB), CGType(CGType), S(S), TH(TH), PT(PT) {
  PAMM_GET_INSTANCE;
  // check for faults in the logic
  if (!TH && (CGType != CallGraphAnalysisType::NORESOLVE)) {
    // no type hierarchy information provided by the user,
    // we need to construct a type hierarchy ourselfes
    this->TH = new LLVMTypeHierarchy(IRDB);
    UserTHInfos = false;
  }
  if (!PT && (CGType == CallGraphAnalysisType::OTF)) {
    // no pointer information provided by the user,
    // we need to construct a points-to infos ourselfes
    this->PT = new LLVMPointsToSet(IRDB);
    UserPTInfos = false;
  }

  if (this->PT == nullptr) {
    llvm::report_fatal_error("LLVMPointsToInfo not passed and "
                             "CallGraphAnalysisType::OTF was not specified.");
  }

  // instantiate the respective resolver type
  Res = makeResolver(IRDB, CGType, *this->TH, *this->PT);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                << "Starting CallGraphAnalysisType: " << CGType);
  VisitedFunctions.reserve(IRDB.getAllFunctions().size());

  for (const auto &EntryPoint : EntryPoints) {
    auto *F = IRDB.getFunctionDefinition(EntryPoint);
    if (F == nullptr) {
      llvm::report_fatal_error("Could not retrieve function for entry point");
    }
    UserEntryPoints.insert(F);
  }

  if (IncludeGlobals) {
    assert(IRDB.getNumberOfModules() == 1 &&
           "IncludeGlobals is currently only supported for WPA");

    const auto *GlobCtor =
        buildCRuntimeGlobalCtorsDtorsModel(*IRDB.getWPAModule());

    FunctionWL.push_back(GlobCtor);
  } else {
    FunctionWL.insert(FunctionWL.end(), UserEntryPoints.begin(),
                      UserEntryPoints.end());
  }

  bool FixpointReached;
  do {
    FixpointReached = true;
    while (!FunctionWL.empty()) {
      const llvm::Function *F = FunctionWL.back();
      FunctionWL.pop_back();
      processFunction(F, *Res, FixpointReached);
    }
    for (const auto &[Callsite, _] : IndirectCalls) {
      FixpointReached &= !constructDynamicCall(Callsite, *Res);
    }
  } while (!FixpointReached);
  for (const auto &[IndirectCall, Targets] : IndirectCalls) {
    if (Targets == 0) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), WARNING)
                    << "No callees found for callsite "
                    << llvmIRToString(IndirectCall));
    }
  }
  REG_COUNTER("CG Vertices", getNumOfVertices(), PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("CG Edges", getNumOfEdges(), PAMM_SEVERITY_LEVEL::Full);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                << "Call graph has been constructed");
}

LLVMBasedICFG::~LLVMBasedICFG() {
  // if we had to compute type hierarchy or points-to information ourselfs,
  // we need to clean up
  if (!UserTHInfos) {
    delete TH;
  }
  if (!UserPTInfos) {
    delete PT;
  }
}

void LLVMBasedICFG::processFunction(const llvm::Function *F, Resolver &Resolver,
                                    bool &FixpointReached) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "Walking in function: " << F->getName().str());
  if (F->isDeclaration() || !VisitedFunctions.insert(F).second) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Function already visited or only declaration: "
                  << F->getName().str());
    return;
  }

  // add a node for function F to the call graph (if not present already)
  vertex_t ThisFunctionVertexDescriptor;
  auto FvmItr = FunctionVertexMap.find(F);
  if (FvmItr != FunctionVertexMap.end()) {
    ThisFunctionVertexDescriptor = FvmItr->second;
  } else {
    ThisFunctionVertexDescriptor =
        boost::add_vertex(VertexProperties(F), CallGraph);
    FunctionVertexMap[F] = ThisFunctionVertexDescriptor;
  }

  // iterate all instructions of the current function
  for (const auto &BB : *F) {
    for (const auto &I : BB) {
      if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
        Resolver.preCall(&I);

        const llvm::CallBase *CS = llvm::cast<llvm::CallBase>(&I);
        set<const llvm::Function *> PossibleTargets;
        // check if function call can be resolved statically
        if (CS->getCalledFunction() != nullptr) {
          PossibleTargets.insert(CS->getCalledFunction());
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Found static call-site: ");
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "  " << llvmIRToString(CS));
        } else {
          // still try to resolve the called function statically
          const llvm::Value *SV = CS->getCalledOperand()->stripPointerCasts();
          const llvm::Function *ValueFunction =
              !SV->hasName() ? nullptr : IRDB.getFunction(SV->getName().str());
          if (ValueFunction) {
            PossibleTargets.insert(ValueFunction);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Found static call-site: " << llvmIRToString(CS));
          } else {
            if (llvm::isa<llvm::InlineAsm>(SV)) {
              continue;
            }
            // the function call must be resolved dynamically
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Found dynamic call-site: ");
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "  " << llvmIRToString(CS));
            IndirectCalls[&I] = 0;
            FixpointReached = false;
            continue;
          }
        }

        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Found " << PossibleTargets.size()
                      << " possible target(s)");

        Resolver.handlePossibleTargets(CS, PossibleTargets);
        // Insert possible target inside the graph and add the link with
        // the current function
        for (const auto &PossibleTarget : PossibleTargets) {
          vertex_t TargetVertex;
          auto TargetFvmItr = FunctionVertexMap.find(PossibleTarget);
          if (TargetFvmItr != FunctionVertexMap.end()) {
            TargetVertex = TargetFvmItr->second;
          } else {
            TargetVertex =
                boost::add_vertex(VertexProperties(PossibleTarget), CallGraph);
            FunctionVertexMap[PossibleTarget] = TargetVertex;
          }
          boost::add_edge(ThisFunctionVertexDescriptor, TargetVertex,
                          EdgeProperties(CS), CallGraph);
        }

        // continue resolving
        FunctionWL.insert(FunctionWL.end(), PossibleTargets.begin(),
                          PossibleTargets.end());

        Resolver.postCall(&I);
      } else {
        Resolver.otherInst(&I);
      }
    }
  }
}

bool LLVMBasedICFG::constructDynamicCall(const llvm::Instruction *I,
                                         Resolver &Resolver) {
  bool NewTargetsFound = false;
  // Find vertex of calling function.
  vertex_t ThisFunctionVertexDescriptor;
  auto FvmItr = FunctionVertexMap.find(I->getFunction());
  if (FvmItr != FunctionVertexMap.end()) {
    ThisFunctionVertexDescriptor = FvmItr->second;
  } else {
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg::get(), ERROR)
        << "constructDynamicCall: Did not find vertex of calling function "
        << I->getFunction()->getName().str() << " at callsite "
        << llvmIRToString(I));
    std::terminate();
  }

  if (llvm::isa<llvm::CallBase>(I)) {
    Resolver.preCall(I);
    const auto *CallSite = llvm::cast<llvm::CallBase>(I);
    set<const llvm::Function *> PossibleTargets;
    // the function call must be resolved dynamically
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Looking into dynamic call-site: ");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "  " << llvmIRToString(I));
    // call the resolve routine
    if (LLVMBasedICFG::isVirtualFunctionCall(CallSite)) {
      PossibleTargets = Resolver.resolveVirtualCall(CallSite);
    } else {
      PossibleTargets = Resolver.resolveFunctionPointer(CallSite);
    }
    if (IndirectCalls.count(I) == 0 ||
        IndirectCalls[I] < PossibleTargets.size()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Found " << PossibleTargets.size() - IndirectCalls[I]
                    << " new possible target(s)");
      IndirectCalls[I] = PossibleTargets.size();
      NewTargetsFound = true;
    }
    if (!NewTargetsFound) {
      return NewTargetsFound;
    }
    // Throw out already found targets
    for (const auto &OE : boost::make_iterator_range(
             boost::out_edges(ThisFunctionVertexDescriptor, CallGraph))) {
      if (CallGraph[OE].CS == I) {
        PossibleTargets.erase(CallGraph[boost::target(OE, CallGraph)].F);
      }
    }
    Resolver.handlePossibleTargets(CallSite, PossibleTargets);
    // Insert possible target inside the graph and add the link with
    // the current function
    for (const auto &PossibleTarget : PossibleTargets) {
      vertex_t TargetVertex;
      auto TargetFvmItr = FunctionVertexMap.find(PossibleTarget);
      if (TargetFvmItr != FunctionVertexMap.end()) {
        TargetVertex = TargetFvmItr->second;
      } else {
        TargetVertex =
            boost::add_vertex(VertexProperties(PossibleTarget), CallGraph);
        FunctionVertexMap[PossibleTarget] = TargetVertex;
      }
      boost::add_edge(ThisFunctionVertexDescriptor, TargetVertex,
                      EdgeProperties(I), CallGraph);
    }

    // continue resolving
    FunctionWL.insert(FunctionWL.end(), PossibleTargets.begin(),
                      PossibleTargets.end());

    Resolver.postCall(I);
  } else {
    Resolver.otherInst(I);
  }
  return NewTargetsFound;
}

std::unique_ptr<Resolver> LLVMBasedICFG::makeResolver(ProjectIRDB &IRDB,
                                                      CallGraphAnalysisType CGT,
                                                      LLVMTypeHierarchy &TH,
                                                      LLVMPointsToInfo &PT) {
  switch (CGType) {
  case (CallGraphAnalysisType::NORESOLVE):
    return make_unique<NOResolver>(IRDB);
    break;
  case (CallGraphAnalysisType::CHA):
    return make_unique<CHAResolver>(IRDB, TH);
    break;
  case (CallGraphAnalysisType::RTA):
    return make_unique<RTAResolver>(IRDB, TH);
    break;
  case (CallGraphAnalysisType::DTA):
    return make_unique<DTAResolver>(IRDB, TH);
    break;
  case (CallGraphAnalysisType::OTF):
    return make_unique<OTFResolver>(IRDB, TH, *this, PT);
    break;
  default:
    llvm::report_fatal_error("Resolver strategy not properly instantiated");
    break;
  }
}

bool LLVMBasedICFG::isIndirectFunctionCall(const llvm::Instruction *N) const {
  const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(N);
  return CallSite && CallSite->isIndirectCall();
}

bool LLVMBasedICFG::isVirtualFunctionCall(const llvm::Instruction *N) const {
  const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(N);
  if (!CallSite) {
    return false;
  }
  // check potential receiver type
  const auto *RecType = getReceiverType(CallSite);
  if (!RecType) {
    return false;
  }
  if (!TH->hasType(RecType)) {
    return false;
  }
  if (!TH->hasVFTable(RecType)) {
    return false;
  }
  return getVFTIndex(CallSite) >= 0;
}

const llvm::Function *LLVMBasedICFG::getFunction(const string &Fun) const {
  return IRDB.getFunction(Fun);
}

std::vector<const llvm::Instruction *>
LLVMBasedICFG::getPredsOf(const llvm::Instruction *Inst) const {
  if (!IgnoreDbgInstructions) {
    if (Inst->getPrevNode()) {
      return {Inst->getPrevNode()};
    }
  } else {
    if (Inst->getPrevNonDebugInstruction()) {
      return {Inst->getPrevNonDebugInstruction()};
    }
  }
  // If we do not have a predecessor yet, look for basic blocks which
  // lead to our instruction in question!

  vector<const llvm::Instruction *> Preds;
  std::transform(llvm::pred_begin(Inst->getParent()),
                 llvm::pred_end(Inst->getParent()), back_inserter(Preds),
                 [](const llvm::BasicBlock *BB) {
                   assert(BB && "BB under analysis was not well formed.");
                   return BB->getTerminator();
                 });

  /// TODO: Add function-local static variable initialization workaround here

  return Preds;
}

std::vector<const llvm::Instruction *>
LLVMBasedICFG::getSuccsOf(const llvm::Instruction *Inst) const {

  vector<const llvm::Instruction *> Successors;
  // case we wish to consider LLVM's debug instructions
  if (!IgnoreDbgInstructions) {
    if (Inst->getNextNode()) {
      Successors.push_back(Inst->getNextNode());
    }
  } else {
    if (Inst->getNextNonDebugInstruction()) {
      Successors.push_back(Inst->getNextNonDebugInstruction());
    }
  }
  if (Successors.empty()) {
    if (const auto *Branch = llvm::dyn_cast<llvm::BranchInst>(Inst);
        Branch && isStaticVariableLazyInitializationBranch(Branch)) {
      // Skip the "already initialized" case, such that the analysis is always
      // aware of the initialized value.
      Successors.push_back(&Branch->getSuccessor(0)->front());

    } else {
      Successors.reserve(Inst->getNumSuccessors() + Successors.size());
      std::transform(llvm::succ_begin(Inst), llvm::succ_end(Inst),
                     std::back_inserter(Successors),
                     [](const llvm::BasicBlock *BB) { return &BB->front(); });
    }
  }

  return Successors;
}

const llvm::Function *LLVMBasedICFG::getFirstGlobalCtorOrNull() const {
  auto it = GlobalCtors.begin();
  if (it != GlobalCtors.end()) {
    return it->second;
  }
  return nullptr;
}
const llvm::Function *LLVMBasedICFG::getLastGlobalDtorOrNull() const {
  auto it = GlobalDtors.rbegin();
  if (it != GlobalDtors.rend()) {
    return it->second;
  }
  return nullptr;
}

set<const llvm::Function *> LLVMBasedICFG::getAllFunctions() const {
  return IRDB.getAllFunctions();
}

boost::container::flat_set<const llvm::Function *>
LLVMBasedICFG::getAllVertexFunctions() const {
  boost::container::flat_set<const llvm::Function *> vertexFuncs;
  vertexFuncs.reserve(FunctionVertexMap.size());
  for (auto v : FunctionVertexMap) {
    vertexFuncs.insert(v.first);
  }
  return vertexFuncs;
}

std::vector<const llvm::Instruction *>
LLVMBasedICFG::getOutEdges(const llvm::Function *F) const {
  auto FunctionMapIt = FunctionVertexMap.find(F);
  if (FunctionMapIt == FunctionVertexMap.end()) {
    return {};
  }

  std::vector<const llvm::Instruction *> Edges;
  for (const auto EdgeIt : boost::make_iterator_range(
           boost::out_edges(FunctionMapIt->second, CallGraph))) {
    auto Edge = CallGraph[EdgeIt];
    Edges.push_back(Edge.CS);
  }

  return Edges;
}

LLVMBasedICFG::OutEdgesAndTargets
LLVMBasedICFG::getOutEdgeAndTarget(const llvm::Function *F) const {
  auto FunctionMapIt = FunctionVertexMap.find(F);
  if (FunctionMapIt == FunctionVertexMap.end()) {
    return {};
  }

  OutEdgesAndTargets Edges;
  for (const auto EdgeIt : boost::make_iterator_range(
           boost::out_edges(FunctionMapIt->second, CallGraph))) {
    auto Edge = CallGraph[EdgeIt];
    auto Target = CallGraph[boost::target(EdgeIt, CallGraph)];
    Edges.insert(std::make_pair(Edge.CS, Target.F));
  }

  return Edges;
}

size_t LLVMBasedICFG::removeEdges(const llvm::Function *F,
                                  const llvm::Instruction *I) {
  auto FunctionMapIt = FunctionVertexMap.find(F);
  if (FunctionMapIt == FunctionVertexMap.end()) {
    return 0;
  }

  size_t EdgesRemoved = 0;
  auto OutEdges = boost::out_edges(FunctionMapIt->second, CallGraph);
  for (auto EdgeIt : boost::make_iterator_range(OutEdges)) {
    auto Edge = CallGraph[EdgeIt];
    if (Edge.CS == I) {
      boost::remove_edge(EdgeIt, CallGraph);
      ++EdgesRemoved;
    }
  }
  return EdgesRemoved;
}

bool LLVMBasedICFG::removeVertex(const llvm::Function *F) {
  auto FunctionMapIt = FunctionVertexMap.find(F);
  if (FunctionMapIt == FunctionVertexMap.end()) {
    return false;
  }

  boost::remove_vertex(FunctionMapIt->second, CallGraph);
  FunctionVertexMap.erase(FunctionMapIt);
  return true;
}

size_t LLVMBasedICFG::getCallerCount(const llvm::Function *F) const {
  auto MapEntry = FunctionVertexMap.find(F);
  if (MapEntry == FunctionVertexMap.end()) {
    return 0;
  }

  auto EdgeIterators = boost::in_edges(MapEntry->second, CallGraph);
  return std::distance(EdgeIterators.first, EdgeIterators.second);
}

set<const llvm::Function *>
LLVMBasedICFG::getCalleesOfCallAt(const llvm::Instruction *N) const {
  if (!llvm::isa<llvm::CallBase>(N)) {
    return {};
  }

  auto MapEntry = FunctionVertexMap.find(N->getFunction());
  if (MapEntry == FunctionVertexMap.end()) {
    return {};
  }

  set<const llvm::Function *> Callees;

  out_edge_iterator EI;
  out_edge_iterator EIEnd;
  for (boost::tie(EI, EIEnd) = boost::out_edges(MapEntry->second, CallGraph);
       EI != EIEnd; ++EI) {
    auto Edge = CallGraph[*EI];
    if (N == Edge.CS) {
      auto Target = boost::target(*EI, CallGraph);
      Callees.insert(CallGraph[Target].F);
    }
  }
  return Callees;
}

void LLVMBasedICFG::forEachCalleeOfCallAt(
    const llvm::Instruction *I,
    llvm::function_ref<void(const llvm::Function *)> Callback) const {
  if (!llvm::isa<llvm::CallInst>(I) && !llvm::isa<llvm::InvokeInst>(I)) {
    return;
  }

  auto MapEntry = FunctionVertexMap.find(I->getFunction());
  if (MapEntry == FunctionVertexMap.end()) {
    return;
  }

  out_edge_iterator EI;

  out_edge_iterator EIEnd;
  for (boost::tie(EI, EIEnd) = boost::out_edges(MapEntry->second, CallGraph);
       EI != EIEnd; ++EI) {
    auto Edge = CallGraph[*EI];
    if (I == Edge.CS) {
      auto Target = boost::target(*EI, CallGraph);
      Callback(CallGraph[Target].F);
    }
  }
}

set<const llvm::Instruction *>
LLVMBasedICFG::getCallersOf(const llvm::Function *F) const {
  set<const llvm::Instruction *> CallersOf;
  auto MapEntry = FunctionVertexMap.find(F);
  if (MapEntry == FunctionVertexMap.end()) {
    return CallersOf;
  }
  in_edge_iterator EI;

  in_edge_iterator EIEnd;
  for (boost::tie(EI, EIEnd) = boost::in_edges(MapEntry->second, CallGraph);
       EI != EIEnd; ++EI) {
    auto Edge = CallGraph[*EI];
    CallersOf.insert(Edge.CS);
  }
  return CallersOf;
}

set<const llvm::Instruction *>
LLVMBasedICFG::getCallsFromWithin(const llvm::Function *F) const {
  set<const llvm::Instruction *> CallSites;
  for (llvm::const_inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F);
       I != E; ++I) {
    if (llvm::isa<llvm::CallBase>(*I)) {
      CallSites.insert(&(*I));
    }
  }
  return CallSites;
}

/**
 * Returns all statements to which a call could return.
 * In the RHS paper, for every call there is just one return site.
 * We, however, use as return site the successor statements, of which
 * there can be many in case of exceptional flow.
 */
set<const llvm::Instruction *>
LLVMBasedICFG::getReturnSitesOfCallAt(const llvm::Instruction *N) const {
  set<const llvm::Instruction *> ReturnSites;
  if (const auto *Invoke = llvm::dyn_cast<llvm::InvokeInst>(N)) {
    ReturnSites.insert(&Invoke->getNormalDest()->front());
    ReturnSites.insert(&Invoke->getUnwindDest()->front());
  } else {
    auto Succs = getSuccsOf(N);
    ReturnSites.insert(Succs.begin(), Succs.end());
  }
  return ReturnSites;
}

void LLVMBasedICFG::collectGlobalCtors() {
  for (const auto *Module : IRDB.getAllModules()) {
    insertGlobalCtorsDtorsImpl(GlobalCtors, Module, "llvm.global_ctors");
    // auto Part = getGlobalCtorsDtorsImpl(Module, "llvm.global_ctors");
    // GlobalCtors.insert(GlobalCtors.begin(), Part.begin(), Part.end());
  }

  // for (auto it = GlobalCtors.cbegin(), end = GlobalCtors.cend(); it != end;
  //      ++it) {
  //   GlobalCtorFn.try_emplace(it->second, it);
  // }
}

void LLVMBasedICFG::collectGlobalDtors() {
  for (const auto *Module : IRDB.getAllModules()) {
    insertGlobalCtorsDtorsImpl(GlobalDtors, Module, "llvm.global_dtors");
    // auto Part = getGlobalCtorsDtorsImpl(Module, "llvm.global_dtors");
    // GlobalDtors.insert(GlobalDtors.begin(), Part.begin(), Part.end());
  }

  // for (auto it = GlobalDtors.cbegin(), end = GlobalDtors.cend(); it != end;
  //      ++it) {
  //   GlobalDtorFn.try_emplace(it->second, it);
  // }
}

void LLVMBasedICFG::collectGlobalInitializers() {
  // get all functions used to initialize global variables
  forEachGlobalCtor([this](auto *GlobalCtor) {
    for (const auto &BB : *GlobalCtor) {
      for (const auto &I : BB) {
        if (auto Call = llvm::dyn_cast<llvm::CallInst>(&I)) {
          GlobalInitializers.push_back(Call->getCalledFunction());
        }
      }
    }
  });
}

llvm::SmallVector<std::pair<llvm::FunctionCallee, llvm::Value *>, 4>
collectRegisteredDtorsForModule(const llvm::Module *Mod) {
  // NOLINTNEXTLINE
  llvm::SmallVector<std::pair<llvm::FunctionCallee, llvm::Value *>, 4>
      RegisteredDtors, RegisteredLocalStaticDtors;

  auto *CxaAtExitFn = Mod->getFunction("__cxa_atexit");
  if (!CxaAtExitFn) {
    return RegisteredDtors;
  }

  auto getConstantBitcastArgument = [](llvm::Value *V) -> llvm::Value * {
    auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(V);
    if (!CE) {
      return V;
    }

    return CE->getOperand(0);
  };

  for (auto *User : CxaAtExitFn->users()) {
    auto *Call = llvm::dyn_cast<llvm::CallBase>(User);
    if (!Call) {
      continue;
    }

    auto *DtorOp = llvm::dyn_cast_or_null<llvm::Function>(
        getConstantBitcastArgument(Call->getArgOperand(0)));
    auto *DtorArgOp = getConstantBitcastArgument(Call->getArgOperand(1));

    if (!DtorOp || !DtorArgOp) {
      continue;
    }

    if (Call->getFunction()->getName().contains("__cxx_global_var_init")) {
      RegisteredDtors.emplace_back(DtorOp, DtorArgOp);
    } else {
      RegisteredLocalStaticDtors.emplace_back(DtorOp, DtorArgOp);
    }
  }

  // Destructors of local static variables are registered last, no matter where
  // they are declared in the source code
  RegisteredDtors.append(RegisteredLocalStaticDtors.begin(),
                         RegisteredLocalStaticDtors.end());

  return RegisteredDtors;
}

std::string getReducedModuleName(const llvm::Module &M) {
  auto Name = M.getName().str();
  if (auto Idx = Name.find_last_of('/'); Idx != std::string::npos) {
    Name.erase(0, Idx + 1);
  }

  return Name;
}

llvm::Function *createDtorCallerForModule(
    llvm::Module *Mod,
    const llvm::SmallVectorImpl<std::pair<llvm::FunctionCallee, llvm::Value *>>
        &RegisteredDtors) {

  auto *PhasarDtorCaller = llvm::cast<llvm::Function>(
      Mod->getOrInsertFunction("__psrGlobalDtorsCaller." +
                                   getReducedModuleName(*Mod),
                               llvm::Type::getVoidTy(Mod->getContext()))
          .getCallee());

  auto *BB =
      llvm::BasicBlock::Create(Mod->getContext(), "entry", PhasarDtorCaller);

  llvm::IRBuilder<> IRB(BB);

  for (auto it = RegisteredDtors.rbegin(), end = RegisteredDtors.rend();
       it != end; ++it) {
    IRB.CreateCall(it->first, {it->second});
  }

  IRB.CreateRetVoid();

  return PhasarDtorCaller;
}

llvm::Function *LLVMBasedICFG::buildCRuntimeGlobalDtorsModel(llvm::Module &M) {

  if (GlobalDtors.size() == 1) {
    return GlobalDtors.begin()->second;
  }

  auto &CTX = M.getContext();
  auto *Cleanup = llvm::cast<llvm::Function>(
      M.getOrInsertFunction("__psrCRuntimeGlobalDtorsModel",
                            llvm::Type::getVoidTy(CTX))
          .getCallee());

  auto *EntryBB = llvm::BasicBlock::Create(CTX, "entry", Cleanup);

  llvm::IRBuilder<> IRB(EntryBB);

  /// Call all statically/dynamically registered dtors

  for (auto [unused, Dtor] : GlobalDtors) {
    assert(Dtor);
    assert(Dtor->arg_empty());
    IRB.CreateCall(Dtor);
  }

  IRB.CreateRetVoid();

  return Cleanup;
}

const llvm::Function *
LLVMBasedICFG::buildCRuntimeGlobalCtorsDtorsModel(llvm::Module &M) {
  collectGlobalCtors();

  collectGlobalDtors();
  collectRegisteredDtors();

  if (!GlobalCleanupFn) {
    GlobalCleanupFn = buildCRuntimeGlobalDtorsModel(M);
  }

  auto &CTX = M.getContext();
  auto *GlobModel = llvm::cast<llvm::Function>(
      M.getOrInsertFunction(GlobalCRuntimeModelName,
                            /*retTy*/
                            llvm::Type::getVoidTy(CTX),
                            /*argc*/
                            llvm::Type::getInt32Ty(CTX),
                            /*argv*/
                            llvm::Type::getInt8PtrTy(CTX)->getPointerTo())
          .getCallee());

  auto *EntryBB = llvm::BasicBlock::Create(CTX, "entry", GlobModel);

  llvm::IRBuilder<> IRB(EntryBB);

  /// First, call all global ctors

  for (auto [unused, Ctor] : GlobalCtors) {
    assert(Ctor != nullptr);
    assert(Ctor->arg_size() == 0);

    IRB.CreateCall(Ctor);
  }

  /// After all ctors have been called, now go for the user-defined entrypoints

  assert(!UserEntryPoints.empty());

  auto callUEntry = [&](llvm::Function *UEntry) {
    switch (UEntry->arg_size()) {
    case 0:
      IRB.CreateCall(UEntry);
      break;
    case 2:
      if (UEntry->getName() != "main") {
        std::cerr << "ERROR: The only entrypoint, where parameters are "
                     "supported, is main\n";
        break;
      }

      IRB.CreateCall(UEntry, {GlobModel->getArg(0), GlobModel->getArg(1)});
      break;
    default:
      std::cerr << "ERROR: Entrypoints with parameters are not supported, "
                   "except for argc and argv in main\n";
      break;
    }

    if (UEntry->getName() == "main") {
      ///  After the main function, we must call all global destructors...
      IRB.CreateCall(GlobalCleanupFn);
    }
  };

  if (UserEntryPoints.size() == 1) {
    auto *MainFn = *UserEntryPoints.begin();
    callUEntry(MainFn);
    IRB.CreateRetVoid();
  } else {

    auto *UEntrySelectorFn = llvm::cast<llvm::Function>(
        M.getOrInsertFunction("__psrCRuntimeUserEntrySelector",
                              llvm::Type::getInt32Ty(CTX))
            .getCallee());

    auto *UEntrySelector = IRB.CreateCall(UEntrySelectorFn, {});

    auto *DefaultBB = llvm::BasicBlock::Create(CTX, "invalid", GlobModel);
    auto *SwitchEnd = llvm::BasicBlock::Create(CTX, "switchEnd", GlobModel);

    auto *UEntrySwitch =
        IRB.CreateSwitch(UEntrySelector, DefaultBB, UserEntryPoints.size());

    IRB.SetInsertPoint(DefaultBB);
    IRB.CreateUnreachable();

    unsigned Idx = 0;

    for (auto *UEntry : UserEntryPoints) {
      auto *BB =
          llvm::BasicBlock::Create(CTX, "call" + UEntry->getName(), GlobModel);
      IRB.SetInsertPoint(BB);
      callUEntry(UEntry);
      IRB.CreateBr(SwitchEnd);

      UEntrySwitch->addCase(IRB.getInt32(Idx), BB);

      ++Idx;
    }

    /// After all user-entries have been called, we are done

    IRB.SetInsertPoint(SwitchEnd);
    IRB.CreateRetVoid();
  }

  return GlobModel;
}

void LLVMBasedICFG::collectRegisteredDtors() {

  for (auto *Mod : IRDB.getAllModules()) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Collect Registered Dtors for Module "
                  << Mod->getName().str());

    auto RegisteredDtors = collectRegisteredDtorsForModule(Mod);

    if (RegisteredDtors.empty()) {
      continue;
    }

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "> Found " << RegisteredDtors.size()
                  << " Registered Dtors");

    auto *RegisteredDtorCaller =
        createDtorCallerForModule(Mod, RegisteredDtors);
    auto it = GlobalDtors.emplace(0, RegisteredDtorCaller);
    // GlobalDtorFn.try_emplace(RegisteredDtorCaller, it);
    GlobalRegisteredDtorsCaller.try_emplace(Mod, RegisteredDtorCaller);
  }
}

/**
 * Returns the set of all nodes that are neither call nor start nodes.
 */
set<const llvm::Instruction *> LLVMBasedICFG::allNonCallStartNodes() const {
  set<const llvm::Instruction *> NonCallStartNodes;
  for (auto *M : IRDB.getAllModules()) {
    for (auto &F : *M) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (!llvm::isa<llvm::CallBase>(&I) && !isStartPoint(&I)) {
            NonCallStartNodes.insert(&I);
          }
        }
      }
    }
  }
  return NonCallStartNodes;
}

void LLVMBasedICFG::mergeWith(const LLVMBasedICFG &Other) {
  using vertex_t = bidigraph_t::vertex_descriptor;
  using vertex_map_t = std::map<vertex_t, vertex_t>;
  vertex_map_t OldToNewVertexMapping;
  boost::associative_property_map<vertex_map_t> VertexMapWrapper(
      OldToNewVertexMapping);
  boost::copy_graph(Other.CallGraph, CallGraph,
                    boost::orig_to_copy(VertexMapWrapper));

  // This vector holds the call-sites that are used to merge the whole-module
  // points-to graphs
  vector<pair<const llvm::CallBase *, const llvm::Function *>> Calls;
  vertex_iterator VIv;

  vertex_iterator VIvEnd;

  vertex_iterator VIu;

  vertex_iterator VIuEnd;
  // Iterate the vertices of this graph 'v'
  for (boost::tie(VIv, VIvEnd) = boost::vertices(CallGraph); VIv != VIvEnd;
       ++VIv) {
    // Iterate the vertices of the other graph 'u'
    for (boost::tie(VIu, VIuEnd) = boost::vertices(CallGraph); VIu != VIuEnd;
         ++VIu) {
      // Check if we have a virtual node that can be replaced with the actual
      // node
      if (CallGraph[*VIv].F == CallGraph[*VIu].F &&
          CallGraph[*VIv].F->isDeclaration() &&
          !CallGraph[*VIu].F->isDeclaration()) {
        in_edge_iterator EI;

        in_edge_iterator EIEnd;
        for (boost::tie(EI, EIEnd) = boost::in_edges(*VIv, CallGraph);
             EI != EIEnd; ++EI) {
          auto Source = boost::source(*EI, CallGraph);
          auto Edge = CallGraph[*EI];
          // This becomes the new edge for this graph to the other graph
          boost::add_edge(Source, *VIu, Edge.CS, CallGraph);
          Calls.emplace_back(llvm::cast<llvm::CallBase>(Edge.CS),
                             CallGraph[*VIu].F);
          // Remove the old edge flowing into the virtual node
          boost::remove_edge(Source, *VIv, CallGraph);
        }
        // Remove the virtual node
        boost::remove_vertex(*VIv, CallGraph);
      }
    }
  }

  // Update the FunctionVertexMap:
  for (const auto &OtherValues : Other.FunctionVertexMap) {
    auto MappingIter = OldToNewVertexMapping.find(OtherValues.second);
    if (MappingIter != OldToNewVertexMapping.end()) {
      FunctionVertexMap.insert(
          make_pair(OtherValues.first, MappingIter->second));
    }
  }

  // Merge the already visited functions
  VisitedFunctions.insert(Other.VisitedFunctions.begin(),
                          Other.VisitedFunctions.end());
  // Merge the points-to graphs
  // WholeModulePTG.mergeWith(Other.WholeModulePTG, Calls);
}

CallGraphAnalysisType LLVMBasedICFG::getCallGraphAnalysisType() const {
  return CGType;
}

void LLVMBasedICFG::print(ostream &OS) const {
  OS << "Call Graph:\n";
  vertex_iterator UI;

  vertex_iterator UIEnd;
  for (boost::tie(UI, UIEnd) = boost::vertices(CallGraph); UI != UIEnd; ++UI) {
    OS << CallGraph[*UI].getFunctionName() << " --> ";
    out_edge_iterator EI;

    out_edge_iterator EIEnd;
    for (boost::tie(EI, EIEnd) = boost::out_edges(*UI, CallGraph); EI != EIEnd;
         ++EI) {
      OS << CallGraph[target(*EI, CallGraph)].getFunctionName() << " ";
    }
    OS << '\n';
  }
}

void LLVMBasedICFG::printAsDot(std::ostream &OS, bool PrintEdgeLabels) const {
  if (PrintEdgeLabels) {
    boost::write_graphviz(OS, CallGraph, VertexWriter<bidigraph_t>(CallGraph),
                          EdgeLabelWriter<bidigraph_t>(CallGraph));
  } else {
    boost::write_graphviz(OS, CallGraph, VertexWriter<bidigraph_t>(CallGraph));
  }
}

nlohmann::json LLVMBasedICFG::getAsJson() const {
  nlohmann::json J;
  vertex_iterator VIv;

  vertex_iterator VIvEnd;
  out_edge_iterator EI;

  out_edge_iterator EIEnd;
  // iterate all graph vertices
  for (boost::tie(VIv, VIvEnd) = boost::vertices(CallGraph); VIv != VIvEnd;
       ++VIv) {
    J[PhasarConfig::JsonCallGraphID()][CallGraph[*VIv].getFunctionName()];
    // iterate all out edges of vertex vi_v
    for (boost::tie(EI, EIEnd) = boost::out_edges(*VIv, CallGraph); EI != EIEnd;
         ++EI) {
      J[PhasarConfig::JsonCallGraphID()][CallGraph[*VIv].getFunctionName()] +=
          CallGraph[boost::target(*EI, CallGraph)].getFunctionName();
    }
  }
  return J;
}

void LLVMBasedICFG::printAsJson(std::ostream &OS) const { OS << getAsJson(); }

nlohmann::json LLVMBasedICFG::exportICFGAsJson() const {
  nlohmann::json J;

  llvm::DenseSet<const llvm::Instruction *> handledCallSites;

  for (const auto *F : getAllFunctions()) {
    for (auto &[From, To] : getAllControlFlowEdges(F)) {
      if (llvm::isa<llvm::UnreachableInst>(From)) {
        continue;
      }

      if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(From)) {
        auto [unused, inserted] = handledCallSites.insert(Call);

        for (const auto *Callee : getCalleesOfCallAt(Call)) {
          if (Callee->isDeclaration()) {
            continue;
          }
          if (inserted) {
            J.push_back({{"from", llvmIRToString(From)},
                         {"to", llvmIRToString(&Callee->front().front())}});
          }

          for (const auto *ExitInst : getAllExitPoints(Callee)) {
            J.push_back({{"from", llvmIRToString(ExitInst)},
                         {"to", llvmIRToString(To)}});
          }
        }

      } else {
        J.push_back(
            {{"from", llvmIRToString(From)}, {"to", llvmIRToString(To)}});
      }
    }
  }

  return J;
}

nlohmann::json LLVMBasedICFG::exportICFGAsSourceCodeJson() const {
  nlohmann::json J;

  auto isRetVoid = [](const llvm::Instruction *Inst) {
    const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(Inst);
    return Ret && !Ret->getReturnValue();
  };

  auto getLastNonEmpty =
      [&](const llvm::Instruction *Inst) -> SourceCodeInfoWithIR {
    if (!isRetVoid(Inst) || !Inst->getPrevNode()) {
      return {getSrcCodeInfoFromIR(Inst), llvmIRToString(Inst)};
    }
    for (const auto *Prev = Inst->getPrevNode(); Prev;
         Prev = Prev->getPrevNode()) {
      auto Src = getSrcCodeInfoFromIR(Prev);
      if (!Src.empty()) {
        return {Src, llvmIRToString(Prev)};
      }
    }

    return {getSrcCodeInfoFromIR(Inst), llvmIRToString(Inst)};
  };

  auto createInterEdges = [&](const llvm::Instruction *CS,
                              const SourceCodeInfoWithIR &From,
                              std::initializer_list<SourceCodeInfoWithIR> Tos) {
    for (const auto *Callee : getCalleesOfCallAt(CS)) {
      if (Callee->isDeclaration()) {
        continue;
      }

      // Call Edge
      auto InterTo = getFirstNonEmpty(&Callee->front());
      J.push_back({{"from", From}, {"to", std::move(InterTo)}});

      // Return Edges
      for (const auto *ExitInst : getAllExitPoints(Callee)) {
        for (const auto &To : Tos) {
          J.push_back({{"from", getLastNonEmpty(ExitInst)}, {"to", To}});
        }
      }
    }
  };

  for (const auto *F : getAllFunctions()) {
    for (const auto &BB : *F) {
      assert(!BB.empty() && "Invalid IR: Empty BasicBlock");
      auto it = BB.begin();
      auto end = BB.end();
      auto From = getFirstNonEmpty(it, end);

      if (it == end) {
        continue;
      }

      const auto *FromInst = &*it;

      ++it;

      // Edges inside the BasicBlock
      for (; it != end; ++it) {
        auto To = getFirstNonEmpty(it, end);
        if (To.empty()) {
          break;
        }

        if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(FromInst)) {
          // Call- and Return Edges
          createInterEdges(FromInst, From, {To});
        } else if (From != To && !isRetVoid(&*it)) {
          // Normal Edge
          J.push_back({{"from", From}, {"to", To}});
        }

        FromInst = &*it;
        From = std::move(To);
      }

      const auto *Term = BB.getTerminator();
      assert(Term && "Invalid IR: BasicBlock without terminating instruction!");

      auto numSuccessors = Term->getNumSuccessors();

      if (numSuccessors == 0) {
        // Return edges already handled
      } else if (const auto *Invoke = llvm::dyn_cast<llvm::InvokeInst>(Term)) {
        // Invoke Edges (they are not handled by the Call edges, because they
        // are always terminator instructions)

        // Note: The unwindDest is never reachable from a return instruction.
        // However, this is how it is modeled in the ICFG at the moment
        createInterEdges(Term,
                         SourceCodeInfoWithIR{getSrcCodeInfoFromIR(Term),
                                              llvmIRToString(Term)},
                         {getFirstNonEmpty(Invoke->getNormalDest()),
                          getFirstNonEmpty(Invoke->getUnwindDest())});
        // Call Edges
      } else {
        // Branch Edges
        for (size_t i = 0; i < numSuccessors; ++i) {
          auto *Succ = Term->getSuccessor(i);
          assert(Succ && !Succ->empty());

          auto To = getFirstNonEmpty(Succ);
          if (From != To) {
            J.push_back({{"from", From}, {"to", std::move(To)}});
          }
        }
      }
    }
  }

  return J;
}

vector<const llvm::Function *> LLVMBasedICFG::getDependencyOrderedFunctions() {
  vector<vertex_t> Vertices;
  vector<const llvm::Function *> Functions;
  dependency_visitor Deps(Vertices);
  boost::depth_first_search(CallGraph, visitor(Deps));
  for (auto V : Vertices) {
    if (!CallGraph[V].F->isDeclaration()) {
      Functions.push_back(CallGraph[V].F);
    }
  }
  return Functions;
}

unsigned LLVMBasedICFG::getNumOfVertices() const {
  return boost::num_vertices(CallGraph);
}

unsigned LLVMBasedICFG::getNumOfEdges() const {
  return boost::num_edges(CallGraph);
}

const llvm::Function *
LLVMBasedICFG::getRegisteredDtorsCallerOrNull(const llvm::Module *Mod) {
  auto it = GlobalRegisteredDtorsCaller.find(Mod);
  if (it != GlobalRegisteredDtorsCaller.end()) {
    return it->second;
  }

  return nullptr;
}

} // namespace psr
