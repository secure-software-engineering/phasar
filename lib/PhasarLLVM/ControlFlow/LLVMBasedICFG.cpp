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

#include <memory>

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/ErrorHandling.h>

#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/DTAResolver.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/OTFResolver.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h>

#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>
#include <phasar/Utils/PAMMMacros.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h>

using namespace psr;
using namespace std;

namespace psr {

struct LLVMBasedICFG::dependency_visitor : boost::default_dfs_visitor {
  std::vector<vertex_t> &vertices;
  dependency_visitor(std::vector<vertex_t> &v) : vertices(v) {}
  template <typename Vertex, typename Graph>
  void finish_vertex(Vertex u, const Graph &g) {
    vertices.push_back(u);
  }
};

LLVMBasedICFG::VertexProperties::VertexProperties(const llvm::Function *F)
    : F(F), FName(F->getName().str()) {}

LLVMBasedICFG::EdgeProperties::EdgeProperties(const llvm::Instruction *I)
    : CS(I),
      // WARNING: Huge cost
      //, ir_code(llvmIRToString(i)),
      IR(""), ID(stoull(getMetaDataID(I))) {}

LLVMBasedICFG::LLVMBasedICFG(LLVMTypeHierarchy &TH, ProjectIRDB &IRDB)
    : TH(TH), IRDB(IRDB) {}

LLVMBasedICFG::LLVMBasedICFG(LLVMTypeHierarchy &TH, ProjectIRDB &IRDB,
                             CallGraphAnalysisType CGType,
                             const set<string> &EntryPoints)
    : CGType(CGType), TH(TH), IRDB(IRDB) {
  PAMM_GET_INSTANCE;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                << "Starting CallGraphAnalysisType: " << CGType);
  VisitedFunctions.reserve(IRDB.getAllFunctions().size());
  unique_ptr<Resolver> Res([CGType, &IRDB, &TH,
                            this]() -> unique_ptr<Resolver> {
    switch (CGType) {
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
      return make_unique<OTFResolver>(IRDB, TH, WholeModulePTG);
      break;
    default:
      llvm::report_fatal_error("Resolver strategy not properly instantiated");
      break;
    }
  }());
  for (auto &EntryPoint : EntryPoints) {
    const llvm::Function *F = IRDB.getFunctionDefinition(EntryPoint);
    if (F == nullptr) {
      llvm::report_fatal_error(
          "Could not retrieve llvm::Function for entry point");
    }
    // PointsToGraph &ptg = *IRDB.getPointsToGraph(EntryPoint);
    // WholeModulePTG.mergeWith(ptg, F);
    constructionWalker(F, Res.get());
  }
  REG_COUNTER("WM-PTG Vertices", WholeModulePTG.getNumOfVertices(),
              PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("WM-PTG Edges", WholeModulePTG.getNumOfEdges(),
              PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("CG Vertices", getNumOfVertices(), PAMM_SEVERITY_LEVEL::Full);
  REG_COUNTER("CG Edges", getNumOfEdges(), PAMM_SEVERITY_LEVEL::Full);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Call graph has been constructed");
}

LLVMBasedICFG::LLVMBasedICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB,
                             const llvm::Module &M,
                             CallGraphAnalysisType CGType,
                             set<string> EntryPoints)
    : CGType(CGType), TH(STH), IRDB(IRDB) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                << "Starting CallGraphAnalysisType: " << CGType);
  VisitedFunctions.reserve(IRDB.getAllFunctions().size());
  if (EntryPoints.empty()) {
    for (auto &F : M) {
      EntryPoints.insert(F.getName().str());
    }
  }
  unique_ptr<Resolver> Res(
      [CGType, &IRDB, &STH, this]() -> unique_ptr<Resolver> {
        switch (CGType) {
        case (CallGraphAnalysisType::CHA):
          return make_unique<CHAResolver>(IRDB, STH);
          break;
        case (CallGraphAnalysisType::RTA):
          return make_unique<RTAResolver>(IRDB, STH);
          break;
        case (CallGraphAnalysisType::DTA):
          return make_unique<DTAResolver>(IRDB, STH);
          break;
        case (CallGraphAnalysisType::OTF):
          return make_unique<OTFResolver>(IRDB, STH, WholeModulePTG);
          break;
        default:
          throw runtime_error("Resolver strategy not properly instantiated");
          break;
        }
      }());
  for (auto &EntryPoint : EntryPoints) {
    const llvm::Function *F = M.getFunction(EntryPoint);
    if (F && !F->isDeclaration()) {
      // PointsToGraph &ptg = *IRDB.getPointsToGraph(EntryPoint);
      // WholeModulePTG.mergeWith(ptg, F);
      constructionWalker(F, Res.get());
    }
  }
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Call graph has been constructed");
}

void LLVMBasedICFG::constructionWalker(const llvm::Function *F,
                                       Resolver *Resolver) {
  PAMM_GET_INSTANCE;
  static bool first_function = true;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Walking in function: " << F->getName().str());
  if (VisitedFunctions.count(F) || F->isDeclaration()) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Function already visited or only declaration: "
                  << F->getName().str());
    return;
  }
  VisitedFunctions.insert(F);

  // add a node for function F to the call graph (if not present already)
  if (!FunctionVertexMap.count(F)) {
    FunctionVertexMap[F] = boost::add_vertex(CallGraph);
    CallGraph[FunctionVertexMap[F]] = VertexProperties(F);
  }

  if (first_function) {
    first_function = false;
    Resolver->firstFunction(F);
  }
  // iterate all instructions of the current function
  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I)) {
        Resolver->preCall(&I);

        llvm::ImmutableCallSite cs(&I);
        set<const llvm::Function *> possible_targets;
        // check if function call can be resolved statically
        if (cs.getCalledFunction() != nullptr) {
          possible_targets.insert(cs.getCalledFunction());
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Found static call-site: ");
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "  " << llvmIRToString(cs.getInstruction()));
        } else {
          // still try to resolve the called function statically
          const llvm::Value *v = cs.getCalledValue();
          const llvm::Value *sv = v->stripPointerCasts();
          if (sv->hasName() && IRDB.getFunction(sv->getName())) {
            possible_targets.insert(IRDB.getFunction(sv->getName()));
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Found static call-site: "
                          << llvmIRToString(cs.getInstruction()));
          } else {
            // the function call must be resolved dynamically
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Found dynamic call-site: ");
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "  " << llvmIRToString(cs.getInstruction()));
            // call the resolve routine
            if (isVirtualFunctionCall(cs.getInstruction())) {
              possible_targets = Resolver->resolveVirtualCall(cs);
            } else {
              possible_targets = Resolver->resolveFunctionPointer(cs);
            }
          }
        }

        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Found " << possible_targets.size()
                      << " possible target(s)");

        Resolver->TreatPossibleTarget(cs, possible_targets);
        // Insert possible target inside the graph and add the link with
        // the current function
        for (auto &possible_target : possible_targets) {
          auto target = possible_target;
          if (!FunctionVertexMap.count(target)) {
            FunctionVertexMap[target] = boost::add_vertex(CallGraph);
            CallGraph[FunctionVertexMap[target]] =
                VertexProperties(possible_target);
          }

          boost::add_edge(FunctionVertexMap[F], FunctionVertexMap[target],
                          EdgeProperties(cs.getInstruction()), CallGraph);
        }

        // continue resolving
        for (auto possible_target : possible_targets) {
          constructionWalker(possible_target, Resolver);
        }

        Resolver->postCall(&I);
      } else {
        Resolver->OtherInst(&I);
      }
    }
  }
}

bool LLVMBasedICFG::isIndirectFunctionCall(const llvm::Instruction *n) const {
  llvm::ImmutableCallSite CS(n);
  return CS.isIndirectCall();
}

bool LLVMBasedICFG::isVirtualFunctionCall(const llvm::Instruction *n) const {
  llvm::ImmutableCallSite CS(n);
  // check potential receiver type
  auto RecType = getReceiverType(CS);
  if (!RecType) {
    return false;
  }
  if (!TH.hasType(RecType)) {
    return false;
  }
  if (!TH.hasVFTable(RecType)) {
    return false;
  }
  return getVFTIndex(CS) >= 0;
}

const llvm::Function *LLVMBasedICFG::getFunction(const string &fun) const {
  return IRDB.getFunction(fun);
}

set<const llvm::Function *> LLVMBasedICFG::getAllFunctions() const {
  return IRDB.getAllFunctions();
}

/**
 * Returns all callee methods for a given call that might be called.
 */
set<const llvm::Function *>
LLVMBasedICFG::getCalleesOfCallAt(const llvm::Instruction *n) const {
  if (llvm::isa<llvm::CallInst>(n) || llvm::isa<llvm::InvokeInst>(n)) {
    set<const llvm::Function *> Callees;
    auto Caller = n->getFunction();
    out_edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) =
             boost::out_edges(FunctionVertexMap.at(Caller), CallGraph);
         ei != ei_end; ++ei) {
      auto source = boost::source(*ei, CallGraph);
      auto edge = CallGraph[*ei];
      auto target = boost::target(*ei, CallGraph);
      if (n == edge.CS) {
        Callees.insert(CallGraph[target].F);
      }
    }
    return Callees;
  } else {
    return {};
  }
}

/**
 * Returns all caller statements/nodes of a given method.
 */
set<const llvm::Instruction *>
LLVMBasedICFG::getCallersOf(const llvm::Function *F) const {
  set<const llvm::Instruction *> CallersOf;
  in_edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) =
           boost::in_edges(FunctionVertexMap.at(F), CallGraph);
       ei != ei_end; ++ei) {
    auto source = boost::source(*ei, CallGraph);
    auto edge = CallGraph[*ei];
    auto target = boost::target(*ei, CallGraph);
    CallersOf.insert(edge.CS);
  }
  return CallersOf;
}

/**
 * Returns all call sites within a given method.
 */
set<const llvm::Instruction *>
LLVMBasedICFG::getCallsFromWithin(const llvm::Function *f) const {
  set<const llvm::Instruction *> CallSites;
  for (llvm::const_inst_iterator I = llvm::inst_begin(f), E = llvm::inst_end(f);
       I != E; ++I) {
    if (llvm::isa<llvm::CallInst>(*I) || llvm::isa<llvm::InvokeInst>(*I)) {
      CallSites.insert(&(*I));
    }
  }
  return CallSites;
}

/**
 * Returns all start points of a given method. There may be
 * more than one start point in case of a backward analysis.
 */
set<const llvm::Instruction *>
LLVMBasedICFG::getStartPointsOf(const llvm::Function *m) const {
  if (!m) {
    return {};
  }
  if (!m->isDeclaration()) {
    return {&m->front().front()};
    // } else if (!getStartPointsOf(getMethod(m->getName().str())).empty()) {
    // return getStartPointsOf(getMethod(m->getName().str()));
  } else {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Could not get starting points of '" << m->getName().str()
                  << "' because it is a declaration");
    return {};
  }
}

set<const llvm::Instruction *>
LLVMBasedICFG::getExitPointsOf(const llvm::Function *fun) const {
  if (!fun->isDeclaration()) {
    return {&fun->back().back()};
  } else {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Could not get exit points of '" << fun->getName().str()
                  << "' which is declaration!");
    return {};
  }
}

/**
 * Returns all statements to which a call could return.
 * In the RHS paper, for every call there is just one return site.
 * We, however, use as return site the successor statements, of which
 * there can be many in case of exceptional flow.
 */
set<const llvm::Instruction *>
LLVMBasedICFG::getReturnSitesOfCallAt(const llvm::Instruction *n) const {
  set<const llvm::Instruction *> ReturnSites;
  if (auto Call = llvm::dyn_cast<llvm::CallInst>(n)) {
    ReturnSites.insert(Call->getNextNode());
  }
  if (auto Invoke = llvm::dyn_cast<llvm::InvokeInst>(n)) {
    ReturnSites.insert(&Invoke->getNormalDest()->front());
    ReturnSites.insert(&Invoke->getUnwindDest()->front());
  }
  return ReturnSites;
}

bool LLVMBasedICFG::isCallStmt(const llvm::Instruction *stmt) const {
  llvm::ImmutableCallSite CS(stmt);
  return CS.isCall();
}

/**
 * Returns the set of all nodes that are neither call nor start nodes.
 */
set<const llvm::Instruction *> LLVMBasedICFG::allNonCallStartNodes() const {
  set<const llvm::Instruction *> NonCallStartNodes;
  for (auto M : IRDB.getAllModules()) {
    for (auto &F : *M) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          if ((!llvm::isa<llvm::CallInst>(&I)) &&
              (!llvm::isa<llvm::InvokeInst>(&I)) && (!isStartPoint(&I))) {
            NonCallStartNodes.insert(&I);
          }
        }
      }
    }
  }
  return NonCallStartNodes;
}

vector<const llvm::Instruction *>
LLVMBasedICFG::getAllInstructionsOfFunction(const string &name) {
  return getAllInstructionsOf(IRDB.getFunctionDefinition(name));
}

const llvm::Instruction *
LLVMBasedICFG::getLastInstructionOf(const string &name) {
  const llvm::Function &f = *IRDB.getFunctionDefinition(name);
  auto last = llvm::inst_end(f);
  last--;
  return &(*last);
}

void LLVMBasedICFG::mergeWith(const LLVMBasedICFG &other) {
  // Copy other graph into this graph
  typedef typename boost::property_map<bidigraph_t, boost::vertex_index_t>::type
      index_map_t;
  // For simple adjacency_list<> this type would be more efficient:
  typedef typename boost::iterator_property_map<
      typename std::vector<LLVMBasedICFG::vertex_t>::iterator, index_map_t,
      LLVMBasedICFG::vertex_t, LLVMBasedICFG::vertex_t &>
      IsoMap;
  // For more generic graphs, one can try typedef std::map<vertex_t, vertex_t>
  // IsoMap;
  vector<LLVMBasedICFG::vertex_t> orig2copy_data(
      boost::num_vertices(other.CallGraph));
  IsoMap mapV = boost::make_iterator_property_map(
      orig2copy_data.begin(), get(boost::vertex_index, other.CallGraph));
  boost::copy_graph(other.CallGraph, CallGraph,
                    boost::orig_to_copy(mapV)); // means g1 += g2
  // This vector hols the call-sites that are used to merge the whole-module
  // points-to graphs
  vector<pair<llvm::ImmutableCallSite, const llvm::Function *>> Calls;
  vertex_iterator vi_v, vi_v_end, vi_u, vi_u_end;
  // Iterate the vertices of this graph 'v'
  for (boost::tie(vi_v, vi_v_end) = boost::vertices(CallGraph);
       vi_v != vi_v_end; ++vi_v) {
    // Iterate the vertices of the other graph 'u'
    for (boost::tie(vi_u, vi_u_end) = boost::vertices(CallGraph);
         vi_u != vi_u_end; ++vi_u) {
      // Check if we have a virtual node that can be replaced with the actual
      // node
      if (CallGraph[*vi_v].F == CallGraph[*vi_u].F &&
          CallGraph[*vi_v].F->isDeclaration() &&
          !CallGraph[*vi_u].F->isDeclaration()) {
        in_edge_iterator ei, ei_end;
        for (boost::tie(ei, ei_end) = boost::in_edges(*vi_v, CallGraph);
             ei != ei_end; ++ei) {
          auto source = boost::source(*ei, CallGraph);
          auto edge = CallGraph[*ei];
          // This becomes the new edge for this graph to the other graph
          boost::add_edge(source, *vi_u, edge.CS, CallGraph);
          Calls.push_back(
              make_pair(llvm::ImmutableCallSite(edge.CS), CallGraph[*vi_u].F));
          // Remove the old edge flowing into the virtual node
          boost::remove_edge(source, *vi_v, CallGraph);
        }
        // Remove the virtual node
        boost::remove_vertex(*vi_v, CallGraph);
      }
    }
  }
  // Update the FunctionVertexMap
  FunctionVertexMap.clear();
  for (boost::tie(vi_v, vi_v_end) = boost::vertices(CallGraph);
       vi_v != vi_v_end; ++vi_v) {
    FunctionVertexMap.insert(make_pair(CallGraph[*vi_v].F, *vi_v));
  }
  // Merge the already visited functions
  VisitedFunctions.insert(other.VisitedFunctions.begin(),
                          other.VisitedFunctions.end());
  // Merge the points-to graphs
  WholeModulePTG.mergeWith(other.WholeModulePTG, Calls);
}

bool LLVMBasedICFG::isPrimitiveFunction(const string &name) {
  for (auto &BB : *IRDB.getFunctionDefinition(name)) {
    for (auto &I : BB) {
      if (llvm::isa<llvm::CallInst>(&I) || llvm::isa<llvm::InvokeInst>(&I)) {
        return false;
      }
    }
  }
  return true;
}

void LLVMBasedICFG::print(ostream &OS) {
  cout << "CallGraph:\n";
  boost::print_graph(
      CallGraph, boost::get(&LLVMBasedICFG::VertexProperties::FName, CallGraph),
      OS);
}

void LLVMBasedICFG::printAsDot(std::ostream &OS) {
  boost::write_graphviz(
      OS, CallGraph,
      boost::make_label_writer(
          boost::get(&LLVMBasedICFG::VertexProperties::FName, CallGraph)),
      boost::make_label_writer(
          boost::get(&LLVMBasedICFG::EdgeProperties::IR, CallGraph)));
}

void LLVMBasedICFG::printInternalPTGAsDot(const string &filename) {
  ofstream ofs(filename);
  boost::write_graphviz(
      ofs, WholeModulePTG.ptg,
      boost::make_label_writer(boost::get(
          &PointsToGraph::VertexProperties::ir_code, WholeModulePTG.ptg)),
      boost::make_label_writer(boost::get(
          &PointsToGraph::EdgeProperties::ir_code, WholeModulePTG.ptg)));
}

nlohmann::json LLVMBasedICFG::getAsJson() const {
  nlohmann::json J;
  vertex_iterator vi_v, vi_v_end;
  out_edge_iterator ei, ei_end;
  // iterate all graph vertices
  for (boost::tie(vi_v, vi_v_end) = boost::vertices(CallGraph);
       vi_v != vi_v_end; ++vi_v) {
    J[PhasarConfig::JsonCallGraphID()][CallGraph[*vi_v].FName];
    // iterate all out edges of vertex vi_v
    for (boost::tie(ei, ei_end) = boost::out_edges(*vi_v, CallGraph);
         ei != ei_end; ++ei) {
      J[PhasarConfig::JsonCallGraphID()][CallGraph[*vi_v].FName] +=
          CallGraph[boost::target(*ei, CallGraph)].FName;
    }
  }
  return J;
}

const PointsToGraph &LLVMBasedICFG::getWholeModulePTG() const {
  return WholeModulePTG;
}

vector<const llvm::Function *> LLVMBasedICFG::getDependencyOrderedFunctions() {
  vector<vertex_t> vertices;
  vector<const llvm::Function *> functions;
  dependency_visitor deps(vertices);
  boost::depth_first_search(CallGraph, visitor(deps));
  for (auto v : vertices) {
    if (!CallGraph[v].F->isDeclaration()) {
      functions.push_back(CallGraph[v].F);
    }
  }
  return functions;
}

unsigned LLVMBasedICFG::getNumOfVertices() {
  return boost::num_vertices(CallGraph);
}

unsigned LLVMBasedICFG::getNumOfEdges() { return boost::num_edges(CallGraph); }

} // namespace psr
