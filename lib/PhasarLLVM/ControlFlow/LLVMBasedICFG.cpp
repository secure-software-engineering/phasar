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
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/Pointer/VTable.h>

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

LLVMBasedICFG::VertexProperties::VertexProperties(const llvm::Function *f,
                                                  bool isDecl)
    : function(f), functionName(f->getName().str()), isDeclaration(isDecl) {}

LLVMBasedICFG::EdgeProperties::EdgeProperties(const llvm::Instruction *i)
    : callsite(i),
      // WARNING: Huge cost
      //, ir_code(llvmIRToString(i)),
      ir_code(""), id(stoull(getMetaDataID(i))) {}

LLVMBasedICFG::LLVMBasedICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB)
    : CH(STH), IRDB(IRDB) {}

LLVMBasedICFG::LLVMBasedICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB,
                             CallGraphAnalysisType CGType,
                             const vector<string> &EntryPoints)
    : CGType(CGType), CH(STH), IRDB(IRDB) {
  PAMM_GET_INSTANCE;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                << "Starting CallGraphAnalysisType: " << CGType);
  VisitedFunctions.reserve(IRDB.getAllFunctions().size());
  unique_ptr<Resolver_t> resolver(
      [CGType, &IRDB, &STH, this]() -> unique_ptr<Resolver_t> {
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
    llvm::Function *F = IRDB.getFunction(EntryPoint);
    if (F == nullptr) {
      throw ios_base::failure(
          "Could not retrieve llvm::Function for entry point");
    }
    PointsToGraph &ptg = *IRDB.getPointsToGraph(EntryPoint);
    WholeModulePTG.mergeWith(ptg, F);
    constructionWalker(F, resolver.get());
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
                             vector<string> EntryPoints)
    : CGType(CGType), CH(STH), IRDB(IRDB) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                << "Starting CallGraphAnalysisType: " << CGType);
  VisitedFunctions.reserve(IRDB.getAllFunctions().size());
  if (EntryPoints.empty()) {
    for (auto &F : M) {
      EntryPoints.push_back(F.getName().str());
    }
  }
  unique_ptr<Resolver_t> resolver(
      [CGType, &IRDB, &STH, this]() -> unique_ptr<Resolver_t> {
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
    llvm::Function *F = M.getFunction(EntryPoint);
    if (F && !F->isDeclaration()) {
      PointsToGraph &ptg = *IRDB.getPointsToGraph(EntryPoint);
      WholeModulePTG.mergeWith(ptg, F);
      constructionWalker(F, resolver.get());
    }
  }
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Call graph has been constructed");
}

void LLVMBasedICFG::constructionWalker(const llvm::Function *F,
                                       Resolver_t *resolver) {
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
  if (!function_vertex_map.count(F->getName().str())) {
    function_vertex_map[F->getName().str()] = boost::add_vertex(cg);
    cg[function_vertex_map[F->getName().str()]] = VertexProperties(F);
  }

  if (first_function) {
    first_function = false;
    resolver->firstFunction(F);
  }

  for (llvm::const_inst_iterator I = llvm::inst_begin(F), E = llvm::inst_end(F);
       I != E; ++I) {
    const llvm::Instruction &Inst = *I;

    if (llvm::isa<llvm::CallInst>(Inst) || llvm::isa<llvm::InvokeInst>(Inst)) {
      resolver->preCall(&Inst);

      llvm::ImmutableCallSite cs(&Inst);
      set<const llvm::Function *> possible_targets;
      // check if function call can be resolved statically
      if (cs.getCalledFunction() != nullptr) {
        possible_targets.insert(cs.getCalledFunction());
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Found static call-site: "
                      << llvmIRToString(cs.getInstruction()));
      } else { // the function call must be resolved dynamically
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Found dynamic call-site: "
                      << llvmIRToString(cs.getInstruction()));
        // call the resolve routine
        set<string> possible_target_names;

        if (isVirtualFunctionCall(cs)) {
          possible_target_names = resolver->resolveVirtualCall(cs);
        } else {
          possible_target_names = resolver->resolveFunctionPointer(cs);
        }

        for (auto &possible_target_name : possible_target_names) {
          if (IRDB.getFunction(possible_target_name)) {
            possible_targets.insert(IRDB.getFunction(possible_target_name));
          }
        }
      }

      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Found " << possible_targets.size()
                    << " possible target(s)");

      resolver->TreatPossibleTarget(cs, possible_targets);
      // Insert possible target inside the graph and add the link with
      // the current function
      for (auto &possible_target : possible_targets) {
        string target_name = possible_target->getName().str();
        if (!function_vertex_map.count(target_name)) {
          function_vertex_map[target_name] = boost::add_vertex(cg);
          cg[function_vertex_map[target_name]] = VertexProperties(
              possible_target, possible_target->isDeclaration());
        }

        boost::add_edge(function_vertex_map[F->getName().str()],
                        function_vertex_map[target_name],
                        EdgeProperties(cs.getInstruction()), cg);
      }

      // continue resolving
      for (auto possible_target : possible_targets) {
        constructionWalker(possible_target, resolver);
      }

      resolver->postCall(&Inst);
    } else {
      resolver->OtherInst(&Inst);
    }
  }
}

bool LLVMBasedICFG::isVirtualFunctionCall(llvm::ImmutableCallSite CS) {
  if (CS.getNumArgOperands() > 0) {
    const llvm::Value *V = CS.getArgOperand(0);
    if (V->getType()->isPointerTy()) {
      if (V->getType()->getPointerElementType()->isStructTy()) {
        string type_name =
            V->getType()->getPointerElementType()->getStructName();

        // get the type name and check if it has a virtual member function
        if (CH.containsType(type_name) && CH.containsVTable(type_name)) {
          VTable vtbl = CH.getVTable(type_name);
          for (const string &Fname : vtbl) {
            const llvm::Function *F = IRDB.getFunction(Fname);

            if (!F) {
              // Is a pure virtual function
              // or there is an error with the function in the module (that can
              // happen)
              return true;
            }

            if (CS.getCalledValue()->getType()->isPointerTy()) {
              if (matchesSignature(F, llvm::dyn_cast<llvm::FunctionType>(
                                          CS.getCalledValue()
                                              ->getType()
                                              ->getPointerElementType()))) {
                return true;
              }
            }
          }
        }
      }
    }
  }
  return false;
}

const llvm::Function *
LLVMBasedICFG::getMethodOf(const llvm::Instruction *stmt) {
  return stmt->getFunction();
}

const llvm::Function *LLVMBasedICFG::getMethod(const string &fun) {
  return IRDB.getFunction(fun);
}

vector<const llvm::Instruction *>
LLVMBasedICFG::getPredsOf(const llvm::Instruction *I) {
  vector<const llvm::Instruction *> Preds;
  if (I->getPrevNode()) {
    Preds.push_back(I->getPrevNode());
  }
  /*
   * If we do not have a predecessor yet, look for basic blocks which
   * lead to our instruction in question!
   */
  if (Preds.empty()) {
    for (auto &BB : *I->getFunction()) {
      if (const llvm::TerminatorInst *T =
              llvm::dyn_cast<llvm::TerminatorInst>(BB.getTerminator())) {
        for (auto successor : T->successors()) {
          if (&*successor->begin() == I) {
            Preds.push_back(T);
          }
        }
      }
    }
  }
  return Preds;
}

vector<const llvm::Instruction *>
LLVMBasedICFG::getSuccsOf(const llvm::Instruction *I) {
  vector<const llvm::Instruction *> Successors;
  if (I->getNextNode())
    Successors.push_back(I->getNextNode());
  if (const llvm::TerminatorInst *T = llvm::dyn_cast<llvm::TerminatorInst>(I)) {
    for (auto successor : T->successors()) {
      Successors.push_back(&*successor->begin());
    }
  }
  return Successors;
}

vector<pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedICFG::getAllControlFlowEdges(const llvm::Function *fun) {
  vector<pair<const llvm::Instruction *, const llvm::Instruction *>> Edges;
  for (auto &BB : *fun) {
    for (auto &I : BB) {
      auto Successors = getSuccsOf(&I);
      for (auto Successor : Successors) {
        Edges.push_back(make_pair(&I, Successor));
      }
    }
  }
  return Edges;
}

set<const llvm::Function *> LLVMBasedICFG::getAllMethods() {
  return IRDB.getAllFunctions();
}

vector<const llvm::Instruction *>
LLVMBasedICFG::getAllInstructionsOf(const llvm::Function *fun) {
  vector<const llvm::Instruction *> Instructions;
  for (auto &BB : *fun) {
    for (auto &I : BB) {
      Instructions.push_back(&I);
    }
  }
  return Instructions;
}

bool LLVMBasedICFG::isExitStmt(const llvm::Instruction *stmt) {
  return llvm::isa<llvm::ReturnInst>(stmt);
}

bool LLVMBasedICFG::isStartPoint(const llvm::Instruction *stmt) {
  return (stmt == &stmt->getFunction()->front().front());
}

bool LLVMBasedICFG::isFallThroughSuccessor(const llvm::Instruction *stmt,
                                           const llvm::Instruction *succ) {
  if (const llvm::BranchInst *B = llvm::dyn_cast<llvm::BranchInst>(stmt)) {
    if (B->isConditional()) {
      return &B->getSuccessor(1)->front() == succ;
    } else {
      return &B->getSuccessor(0)->front() == succ;
    }
  }
  return false;
}

bool LLVMBasedICFG::isBranchTarget(const llvm::Instruction *stmt,
                                   const llvm::Instruction *succ) {
  if (const llvm::TerminatorInst *T =
          llvm::dyn_cast<llvm::TerminatorInst>(stmt)) {
    for (auto successor : T->successors()) {
      if (&*successor->begin() == succ) {
        return true;
      }
    }
  }
  return false;
}

string LLVMBasedICFG::getStatementId(const llvm::Instruction *stmt) {
  return llvm::cast<llvm::MDString>(
             stmt->getMetadata(MetaDataKind)->getOperand(0))
      ->getString()
      .str();
}

string LLVMBasedICFG::getMethodName(const llvm::Function *fun) {
  return fun->getName().str();
}

/**
 * Returns all callee methods for a given call that might be called.
 */
set<const llvm::Function *>
LLVMBasedICFG::getCalleesOfCallAt(const llvm::Instruction *n) {
  auto &lg = lg::get();
  if (llvm::isa<llvm::CallInst>(n) || llvm::isa<llvm::InvokeInst>(n)) {
    llvm::ImmutableCallSite CS(n);
    set<const llvm::Function *> Callees;
    string CallerName = CS->getFunction()->getName().str();
    out_edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) =
             boost::out_edges(function_vertex_map[CallerName], cg);
         ei != ei_end; ++ei) {
      auto source = boost::source(*ei, cg);
      auto edge = cg[*ei];
      auto target = boost::target(*ei, cg);
      if (CS.getInstruction() == edge.callsite) {
        // cout << "Name: " << cg[target].functionName << endl;
        if (IRDB.getFunction(cg[target].functionName)) {
          Callees.insert(IRDB.getFunction(cg[target].functionName));
        } else {
          // Either we have a special function called like glibc- or
          // llvm intrinsic functions or a function that is defined in
          // a thrid party library which we have no access to.
          Callees.insert(cg[target].function);
        }
      }
    }
    return Callees;
  } else {
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg, ERROR)
        << "Found instruction that is neither CallInst nor InvokeInst\n");
    return {};
  }
}

/**
 * Returns all caller statements/nodes of a given method.
 */
set<const llvm::Instruction *>
LLVMBasedICFG::getCallersOf(const llvm::Function *m) {
  set<const llvm::Instruction *> CallersOf;
  in_edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) =
           boost::in_edges(function_vertex_map[m->getName().str()], cg);
       ei != ei_end; ++ei) {
    auto source = boost::source(*ei, cg);
    auto edge = cg[*ei];
    auto target = boost::target(*ei, cg);
    CallersOf.insert(edge.callsite);
  }
  return CallersOf;
}

/**
 * Returns all call sites within a given method.
 */
set<const llvm::Instruction *>
LLVMBasedICFG::getCallsFromWithin(const llvm::Function *f) {
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
LLVMBasedICFG::getStartPointsOf(const llvm::Function *m) {
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
LLVMBasedICFG::getExitPointsOf(const llvm::Function *fun) {
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
LLVMBasedICFG::getReturnSitesOfCallAt(const llvm::Instruction *n) {
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

bool LLVMBasedICFG::isCallStmt(const llvm::Instruction *stmt) {
  return llvm::isa<llvm::CallInst>(stmt) || llvm::isa<llvm::InvokeInst>(stmt);
}

/**
 * Returns the set of all nodes that are neither call nor start nodes.
 */
set<const llvm::Instruction *> LLVMBasedICFG::allNonCallStartNodes() {
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
  return getAllInstructionsOf(IRDB.getFunction(name));
}

const llvm::Instruction *
LLVMBasedICFG::getLastInstructionOf(const string &name) {
  const llvm::Function &f = *IRDB.getFunction(name);
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
  vector<LLVMBasedICFG::vertex_t> orig2copy_data(boost::num_vertices(other.cg));
  IsoMap mapV = boost::make_iterator_property_map(
      orig2copy_data.begin(), get(boost::vertex_index, other.cg));
  boost::copy_graph(other.cg, cg, boost::orig_to_copy(mapV)); // means g1 += g2
  // This vector hols the call-sites that are used to merge the whole-module
  // points-to graphs
  vector<pair<llvm::ImmutableCallSite, const llvm::Function *>> Calls;
  vertex_iterator vi_v, vi_v_end, vi_u, vi_u_end;
  // Iterate the vertices of this graph 'v'
  for (boost::tie(vi_v, vi_v_end) = boost::vertices(cg); vi_v != vi_v_end;
       ++vi_v) {
    // Iterate the vertices of the other graph 'u'
    for (boost::tie(vi_u, vi_u_end) = boost::vertices(cg); vi_u != vi_u_end;
         ++vi_u) {
      // Check if we have a virtual node that can be replaced with the actual
      // node
      if (cg[*vi_v].functionName == cg[*vi_u].functionName &&
          cg[*vi_v].isDeclaration && !cg[*vi_u].isDeclaration) {
        in_edge_iterator ei, ei_end;
        for (boost::tie(ei, ei_end) = boost::in_edges(*vi_v, cg); ei != ei_end;
             ++ei) {
          auto source = boost::source(*ei, cg);
          auto edge = cg[*ei];
          // This becomes the new edge for this graph to the other graph
          boost::add_edge(source, *vi_u, edge.callsite, cg);
          Calls.push_back(make_pair(llvm::ImmutableCallSite(edge.callsite),
                                    cg[*vi_u].function));
          // Remove the old edge flowing into the virtual node
          boost::remove_edge(source, *vi_v, cg);
        }
        // Remove the virtual node
        boost::remove_vertex(*vi_v, cg);
      }
    }
  }
  // Update the function_vertex_map
  function_vertex_map.clear();
  for (boost::tie(vi_v, vi_v_end) = boost::vertices(cg); vi_v != vi_v_end;
       ++vi_v) {
    function_vertex_map.insert(make_pair(cg[*vi_v].functionName, *vi_v));
  }
  // Merge the already visited functions
  VisitedFunctions.insert(other.VisitedFunctions.begin(),
                          other.VisitedFunctions.end());
  // Merge the points-to graphs
  WholeModulePTG.mergeWith(other.WholeModulePTG, Calls);
}

bool LLVMBasedICFG::isPrimitiveFunction(const string &name) {
  for (auto &BB : *IRDB.getFunction(name)) {
    for (auto &I : BB) {
      if (llvm::isa<llvm::CallInst>(&I) || llvm::isa<llvm::InvokeInst>(&I)) {
        return false;
      }
    }
  }
  return true;
}

void LLVMBasedICFG::print() {
  cout << "CallGraph:\n";
  boost::print_graph(
      cg, boost::get(&LLVMBasedICFG::VertexProperties::functionName, cg));
}

void LLVMBasedICFG::printAsDot(const string &filename) {
  ofstream ofs(filename);
  boost::write_graphviz(
      ofs, cg,
      boost::make_label_writer(
          boost::get(&LLVMBasedICFG::VertexProperties::functionName, cg)),
      boost::make_label_writer(
          boost::get(&LLVMBasedICFG::EdgeProperties::ir_code, cg)));
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

json LLVMBasedICFG::getAsJson() {
  json J;
  vertex_iterator vi_v, vi_v_end;
  out_edge_iterator ei, ei_end;
  // iterate all graph vertices
  for (boost::tie(vi_v, vi_v_end) = boost::vertices(cg); vi_v != vi_v_end;
       ++vi_v) {
    J[JsonCallGraphID][cg[*vi_v].functionName];
    // iterate all out edges of vertex vi_v
    for (boost::tie(ei, ei_end) = boost::out_edges(*vi_v, cg); ei != ei_end;
         ++ei) {
      J[JsonCallGraphID][cg[*vi_v].functionName] +=
          cg[boost::target(*ei, cg)].functionName;
    }
  }
  return J;
}

PointsToGraph &LLVMBasedICFG::getWholeModulePTG() { return WholeModulePTG; }

vector<string> LLVMBasedICFG::getDependencyOrderedFunctions() {
  vector<vertex_t> vertices;
  vector<string> functionNames;
  dependency_visitor deps(vertices);
  boost::depth_first_search(cg, visitor(deps));
  for (auto v : vertices) {
    if (!cg[v].isDeclaration) {
      functionNames.push_back(cg[v].functionName);
    }
  }
  return functionNames;
}

unsigned LLVMBasedICFG::getNumOfVertices() { return boost::num_vertices(cg); }

unsigned LLVMBasedICFG::getNumOfEdges() { return boost::num_edges(cg); }

} // namespace psr
