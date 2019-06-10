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
#include <llvm/Analysis/CFLSteensAliasAnalysis.h>
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

#include <phasar/PhasarLLVM/Pointer/PointsToGraph.h>

#include <phasar/Utils/GraphExtensions.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>
#include <phasar/Utils/PAMMMacros.h>

using namespace std;
using namespace psr;

namespace psr {

struct PointsToGraph::allocation_site_dfs_visitor : boost::default_dfs_visitor {
  // collect the allocation sites that are found
  std::set<const llvm::Value *> &allocation_sites;
  // keeps track of the current path
  std::vector<vertex_t> visitor_stack;
  // the call stack that can be matched against the visitor stack
  const std::vector<const llvm::Instruction *> &call_stack;

  allocation_site_dfs_visitor(
      std::set<const llvm::Value *> &allocation_sizes,
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
            llvm::dyn_cast<llvm::AllocaInst>(g[u].value)) {
      // If the call stack is empty, we completely ignore the calling context
      if (matches_stack(g) || call_stack.empty()) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Found stack allocation: " << llvmIRToString(Alloc));
        allocation_sites.insert(g[u].value);
      }
    }
    // check for heap allocation
    if (llvm::isa<llvm::CallInst>(g[u].value) ||
        llvm::isa<llvm::InvokeInst>(g[u].value)) {
      llvm::ImmutableCallSite CS(g[u].value);
      if (CS.getCalledFunction() != nullptr &&
          HeapAllocationFunctions.count(
              CS.getCalledFunction()->getName().str())) {
        // If the call stack is empty, we completely ignore the calling
        // context
        if (matches_stack(g) || call_stack.empty()) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "Found heap allocation: "
                        << llvmIRToString(CS.getInstruction()));
          allocation_sites.insert(g[u].value);
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
      if (g[e.first].value == nullptr)
        continue;
      if (g[e.first].value !=
          call_stack[call_stack.size() - call_stack_idx - 1]) {
        return false;
      }
      call_stack_idx++;
    }
    return true;
  }
};

struct PointsToGraph::reachability_dfs_visitor : boost::default_dfs_visitor {
  std::set<vertex_t> &points_to_set;
  reachability_dfs_visitor(set<vertex_t> &result) : points_to_set(result) {}
  template <typename Vertex, typename Graph>
  void finish_vertex(Vertex u, const Graph &g) {
    points_to_set.insert(u);
  }
};

void PrintResults(const char *Msg, bool P, const llvm::Value *V1,
                  const llvm::Value *V2, const llvm::Module *M) {
  if (P) {
    std::string o1, o2;
    {
      llvm::raw_string_ostream os1(o1), os2(o2);
      V1->printAsOperand(os1, true, M);
      V2->printAsOperand(os2, true, M);
    }

    if (o2 < o1)
      std::swap(o1, o2);
    llvm::errs() << "  " << Msg << ":\t" << o1 << ", " << o2 << "\n";
  }
}

void PrintModRefResults(const char *Msg, bool P, llvm::Instruction *I,
                        llvm::Value *Ptr, llvm::Module *M) {
  if (P) {
    llvm::errs() << "  " << Msg << ":  Ptr: ";
    Ptr->printAsOperand(llvm::errs(), true, M);
    llvm::errs() << "\t<->" << *I << '\n';
  }
}

void PrintModRefResults(const char *Msg, bool P, llvm::CallSite CSA,
                        llvm::CallSite CSB, llvm::Module *M) {
  if (P) {
    llvm::errs() << "  " << Msg << ": " << *CSA.getInstruction() << " <-> "
                 << *CSB.getInstruction() << '\n';
  }
}

void PrintLoadStoreResults(const char *Msg, bool P, const llvm::Value *V1,
                           const llvm::Value *V2, const llvm::Module *M) {
  if (P) {
    llvm::errs() << "  " << Msg << ": " << *V1 << " <-> " << *V2 << '\n';
  }
}

// points-to graph internal stuff

PointsToGraph::VertexProperties::VertexProperties(const llvm::Value *v)
    : value(v) {
  // WARNING: equivalent to llvmIRToString
  // WARNING 2 : really really really slow (yes it is)
  // // save the ir code
  // llvm::raw_string_ostream rso(ir_code);
  // value->print(rso);
  // // retrieve the id
  // if (const llvm::Instruction *inst =
  //         llvm::dyn_cast<llvm::Instruction>(value)) {
  //   id = stoull(llvm::cast<llvm::MDString>(
  //                   inst->getMetadata(MetaDataKind)->getOperand(0))
  //                   ->getString()
  //                   .str());
  // }
}

PointsToGraph::EdgeProperties::EdgeProperties(const llvm::Value *v) : value(v) {
  // save the ir code
  // WARNING: equivalent to llvmIRToString
  // WARNING 2 : really really really slow (yes it is)
  // if (v) {
  //   llvm::raw_string_ostream rso(ir_code);
  //   value->print(rso);
  //   // retrieve the id
  //   if (const llvm::Instruction *inst =
  //           llvm::dyn_cast<llvm::Instruction>(value)) {
  //     id = stoull(llvm::cast<llvm::MDString>(
  //                     inst->getMetadata(MetaDataKind)->getOperand(0))
  //                     ->getString()
  //                     .str());
  //   }
  // }
}

// points-to graph stuff

const set<string> PointsToGraph::HeapAllocationFunctions = {
    "_Znwm", "_Znam", "malloc", "calloc", "realloc"};

const map<string, PointerAnalysisType> StringToPointerAnalysisType = {
    {"CFLSteens", PointerAnalysisType::CFLSteens},
    {"CFLAnders", PointerAnalysisType::CFLAnders}};

const map<PointerAnalysisType, string> PointerAnalysisTypeToString = {
    {PointerAnalysisType::CFLSteens, "CFLSteens"},
    {PointerAnalysisType::CFLAnders, "CFLAnders"}};

PointsToGraph::PointsToGraph(llvm::AAResults &AA, llvm::Function *F,
                             bool onlyConsiderMustAlias) {
  PAMM_GET_INSTANCE;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Analyzing function: " << F->getName().str());
  ContainedFunctions.insert(F->getName().str());
  bool PrintNoAlias, PrintMayAlias, PrintPartialAlias, PrintMustAlias;
  PrintNoAlias = PrintMayAlias = PrintPartialAlias = PrintMustAlias = true;
  // ModRef information
  bool PrintNoModRef, PrintMod, PrintRef, PrintModRef;
  PrintNoModRef = PrintMod = PrintRef = PrintModRef = false;
  const llvm::DataLayout &DL = F->getParent()->getDataLayout();
  llvm::SetVector<llvm::Value *> Pointers;
  llvm::SmallSetVector<llvm::CallSite, 16> CallSites;
  llvm::SetVector<llvm::Value *> Loads;
  llvm::SetVector<llvm::Value *> Stores;

  for (auto &I : F->args())
    if (I.getType()->isPointerTy()) // Add all pointer arguments.
      Pointers.insert(&I);

  for (llvm::inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    if (I->getType()->isPointerTy()) // Add all pointer instructions.
      Pointers.insert(&*I);
    if (llvm::isa<llvm::LoadInst>(&*I))
      Loads.insert(&*I);
    if (llvm::isa<llvm::StoreInst>(&*I))
      Stores.insert(&*I);
    llvm::Instruction &Inst = *I;
    if (auto CS = llvm::CallSite(&Inst)) {
      llvm::Value *Callee = CS.getCalledValue();
      // Skip actual functions for direct function calls.
      if (!llvm::isa<llvm::Function>(Callee) && isInterestingPointer(Callee))
        Pointers.insert(Callee);
      // Consider formals.
      for (llvm::Use &DataOp : CS.data_ops())
        if (isInterestingPointer(DataOp))
          Pointers.insert(DataOp);
      CallSites.insert(CS);
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

  //  llvm::errs() << "Function: " << F->getName() << ": " << Pointers.size()
  //               << " pointers, " << CallSites.size() << " call sites\n";

  // make vertices for all pointers
  for (auto pointer : Pointers) {
    value_vertex_map[pointer] = boost::add_vertex(ptg);
    ptg[value_vertex_map[pointer]] = VertexProperties(pointer);
  }
  // iterate over the worklist, and run the full (n^2)/2 disambiguations
  for (llvm::SetVector<llvm::Value *>::iterator I1 = Pointers.begin(),
                                                E = Pointers.end();
       I1 != E; ++I1) {
    uint64_t I1Size = llvm::MemoryLocation::UnknownSize;
    llvm::Type *I1ElTy =
        llvm::cast<llvm::PointerType>((*I1)->getType())->getElementType();
    if (I1ElTy->isSized())
      I1Size = DL.getTypeStoreSize(I1ElTy);
    for (llvm::SetVector<llvm::Value *>::iterator I2 = Pointers.begin();
         I2 != I1; ++I2) {
      uint64_t I2Size = llvm::MemoryLocation::UnknownSize;
      llvm::Type *I2ElTy =
          llvm::cast<llvm::PointerType>((*I2)->getType())->getElementType();
      if (I2ElTy->isSized())
        I2Size = DL.getTypeStoreSize(I2ElTy);
      if (!onlyConsiderMustAlias) {
        switch (AA.alias(llvm::MemoryLocation(*I1, I1Size),
                         llvm::MemoryLocation(*I2, I2Size))) {
        case llvm::NoAlias:
          // PrintResults("NoAlias", PrintNoAlias, *I1, *I2, F->getParent());
          break;
        case llvm::MayAlias:
          // PrintResults("MayAlias", PrintMayAlias, *I1, *I2,
          // F->getParent());
          boost::add_edge(value_vertex_map[*I1], value_vertex_map[*I2], ptg);
          break;
        case llvm::PartialAlias:
          // PrintResults("PartialAlias", PrintPartialAlias, *I1, *I2,
          // 						 F->getParent());
          boost::add_edge(value_vertex_map[*I1], value_vertex_map[*I2], ptg);
          break;
        case llvm::MustAlias:
          // PrintResults("MustAlias", PrintMustAlias, *I1, *I2,
          //              F->getParent());
          boost::add_edge(value_vertex_map[*I1], value_vertex_map[*I2], ptg);
          break;
        default:
          // Do nothing
          break;
        }
      } else {
        if (AA.alias(llvm::MemoryLocation(*I1, I1Size),
                     llvm::MemoryLocation(*I2, I2Size)) == llvm::MustAlias) {
          // PrintResults("MustAlias", PrintMustAlias, *I1, *I2,
          //              F->getParent());
          boost::add_edge(value_vertex_map[*I1], value_vertex_map[*I2], ptg);
        }
      }
    }
  }
}

PointsToGraph::PointsToGraph(vector<string> fnames) {
  ContainedFunctions.insert(fnames.begin(), fnames.end());
}

bool PointsToGraph::isInterestingPointer(llvm::Value *V) {
  return V->getType()->isPointerTy() &&
         !llvm::isa<llvm::ConstantPointerNull>(V);
}

vector<pair<unsigned, const llvm::Value *>>
PointsToGraph::getPointersEscapingThroughParams() {
  vector<pair<unsigned, const llvm::Value *>> escaping_pointers;
  for (pair<vertex_iterator_t, vertex_iterator_t> vp = boost::vertices(ptg);
       vp.first != vp.second; ++vp.first) {
    if (const llvm::Argument *arg =
            llvm::dyn_cast<llvm::Argument>(ptg[*vp.first].value)) {
      escaping_pointers.push_back(make_pair(arg->getArgNo(), arg));
    }
  }
  return escaping_pointers;
}

vector<const llvm::Value *>
PointsToGraph::getPointersEscapingThroughReturns() const {
  vector<const llvm::Value *> escaping_pointers;
  for (pair<vertex_iterator_t, vertex_iterator_t> vp = boost::vertices(ptg);
       vp.first != vp.second; ++vp.first) {
    for (auto user : ptg[*vp.first].value->users()) {
      if (llvm::isa<llvm::ReturnInst>(user)) {
        escaping_pointers.push_back(ptg[*vp.first].value);
      }
    }
  }
  return escaping_pointers;
}

vector<const llvm::Value *>
PointsToGraph::getPointersEscapingThroughReturnsForFunction(
    const llvm::Function *F) const {
  vector<const llvm::Value *> escaping_pointers;
  for (pair<vertex_iterator_t, vertex_iterator_t> vp = boost::vertices(ptg);
       vp.first != vp.second; ++vp.first) {
    for (auto user : ptg[*vp.first].value->users()) {
      if (auto R = llvm::dyn_cast<llvm::ReturnInst>(user)) {
        if (R->getFunction() == F)
          escaping_pointers.push_back(ptg[*vp.first].value);
      }
    }
  }
  return escaping_pointers;
}

set<const llvm::Value *> PointsToGraph::getReachableAllocationSites(
    const llvm::Value *V, vector<const llvm::Instruction *> CallStack) {
  set<const llvm::Value *> alloc_sites;
  allocation_site_dfs_visitor alloc_vis(alloc_sites, CallStack);
  vector<boost::default_color_type> color_map(boost::num_vertices(ptg));
  boost::depth_first_visit(
      ptg, value_vertex_map[V], alloc_vis,
      boost::make_iterator_property_map(color_map.begin(),
                                        boost::get(boost::vertex_index, ptg),
                                        color_map[0]));
  return alloc_sites;
}

bool PointsToGraph::containsValue(llvm::Value *V) {
  pair<vertex_iterator_t, vertex_iterator_t> vp;
  for (vp = boost::vertices(ptg); vp.first != vp.second; ++vp.first)
    if (ptg[*vp.first].value == V)
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

set<const llvm::Value *> PointsToGraph::getPointsToSet(const llvm::Value *V) {
  PAMM_GET_INSTANCE;
  INC_COUNTER("[Calls] getPointsToSet", 1, PAMM_SEVERITY_LEVEL::Full);
  START_TIMER("PointsTo-Set Computation", PAMM_SEVERITY_LEVEL::Full);
  set<vertex_t> reachable_vertices;
  reachability_dfs_visitor vis(reachable_vertices);
  vector<boost::default_color_type> color_map(boost::num_vertices(ptg));
  boost::depth_first_visit(
      ptg, value_vertex_map[V], vis,
      boost::make_iterator_property_map(color_map.begin(),
                                        boost::get(boost::vertex_index, ptg),
                                        color_map[0]));
  set<const llvm::Value *> result;
  for (auto vertex : reachable_vertices) {
    result.insert(ptg[vertex].value);
  }
  PAUSE_TIMER("PointsTo-Set Computation", PAMM_SEVERITY_LEVEL::Full);
  ADD_TO_HISTOGRAM("Points-to", result.size(), 1, PAMM_SEVERITY_LEVEL::Full);
  return result;
}

bool PointsToGraph::representsSingleFunction() {
  return ContainedFunctions.size() == 1;
}

void PointsToGraph::print() {
  cout << "PointsToGraph for ";
  for (const auto &fname : ContainedFunctions) {
    cout << fname << " ";
  }
  cout << "\n";
  boost::print_graph(
      ptg, boost::get(&PointsToGraph::VertexProperties::ir_code, ptg));
}

void PointsToGraph::print() const {
  cout << "PointsToGraph for ";
  for (const auto &fname : ContainedFunctions) {
    cout << fname << " ";
  }
  cout << "\n";
  boost::print_graph(
      ptg, boost::get(&PointsToGraph::VertexProperties::ir_code, ptg));
}

void PointsToGraph::printAsDot(const string &filename) {
  ofstream ofs(filename);
  boost::write_graphviz(ofs, ptg,
                        boost::make_label_writer(boost::get(
                            &PointsToGraph::VertexProperties::ir_code, ptg)),
                        boost::make_label_writer(boost::get(
                            &PointsToGraph::EdgeProperties::ir_code, ptg)));
}

json PointsToGraph::getAsJson() {
  json J;
  vertex_iterator vi_v, vi_v_end;
  out_edge_iterator ei, ei_end;
  // iterate all graph vertices
  for (boost::tie(vi_v, vi_v_end) = boost::vertices(ptg); vi_v != vi_v_end;
       ++vi_v) {
    J[JsonPointToGraphID][llvmIRToString(ptg[*vi_v].value)];
    // iterate all out edges of vertex vi_v
    for (boost::tie(ei, ei_end) = boost::out_edges(*vi_v, ptg); ei != ei_end;
         ++ei) {
      J[JsonPointToGraphID][llvmIRToString(ptg[*vi_v].value)] +=
          llvmIRToString(ptg[boost::target(*ei, ptg)].value);
    }
  }
  return J;
}

void PointsToGraph::printValueVertexMap() {
  for (const auto &entry : value_vertex_map) {
    cout << entry.first << " <---> " << entry.second << endl;
  }
}

void PointsToGraph::mergeWith(const PointsToGraph &Other,
                              const llvm::Function *F) {
  if (!ContainedFunctions.count(F->getName().str())) {
    ContainedFunctions.insert(F->getName().str());
    copy_graph<PointsToGraph::graph_t, PointsToGraph::vertex_t>(ptg, Other.ptg);
    value_vertex_map.clear();
    vertex_iterator_t vi, vi_end;
    for (boost::tie(vi, vi_end) = boost::vertices(ptg); vi != vi_end; ++vi) {
      value_vertex_map.insert(make_pair(ptg[*vi].value, *vi));
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
      // Other.value_vertex_map.count(Formal) << endl;
      // Check if the value is of type pointer, therefore it must be contained
      // in the value_vertex_maps
      if (value_vertex_map.count(Call.first.getArgOperand(i)) &&
          Other.value_vertex_map.count(Formal)) {
        v_in_g1_u_in_g2.push_back(
            tuple<PointsToGraph::vertex_t, PointsToGraph::vertex_t,
                  const llvm::Instruction *>(
                value_vertex_map[Call.first.getArgOperand(i)],
                Other.value_vertex_map.at(Formal),
                Call.first.getInstruction()));
      }
    }

    for (auto Formal :
         Other.getPointersEscapingThroughReturnsForFunction(Call.second)) {
      if (value_vertex_map.count(Call.first.getInstruction()) &&
          Other.value_vertex_map.count(Formal)) {
        v_in_g1_u_in_g2.push_back(
            tuple<PointsToGraph::vertex_t, PointsToGraph::vertex_t,
                  const llvm::Instruction *>(
                value_vertex_map[Call.first.getInstruction()],
                Other.value_vertex_map.at(Formal),
                Call.first.getInstruction()));
      }
    }
    ContainedFunctions.insert(Call.second->getName().str());
  }
  merge_graphs<PointsToGraph::graph_t, PointsToGraph::vertex_t,
               PointsToGraph::EdgeProperties, const llvm::Instruction *>(
      ptg, Other.ptg, v_in_g1_u_in_g2);
  value_vertex_map.clear();
  vertex_iterator_t vi, vi_end;
  for (boost::tie(vi, vi_end) = boost::vertices(ptg); vi != vi_end; ++vi) {
    value_vertex_map.insert(make_pair(ptg[*vi].value, *vi));
  }
}

void PointsToGraph::mergeWith(PointsToGraph &Other, llvm::ImmutableCallSite CS,
                              const llvm::Function *F) {
  // Check if points-to graph of F is already within 'this' whole module
  // points-to graph
  if (ContainedFunctions.count(F->getName().str())) {
    for (unsigned i = 0; i < CS.getNumArgOperands(); ++i) {
      auto Formal = getNthFunctionArgument(F, i);
      // Only draw the edges, when these values are of type pointer and
      // therefore contained in value_vertex_map
      if (value_vertex_map.count(CS.getArgOperand(i)) &&
          value_vertex_map.count(Formal)) {
        boost::add_edge(value_vertex_map[CS.getArgOperand(i)],
                        value_vertex_map[Formal], CS.getInstruction(), ptg);
      }
    }

    for (auto Formal : getPointersEscapingThroughReturnsForFunction(F)) {
      if (value_vertex_map.count(CS.getInstruction()) &&
          value_vertex_map.count(Formal)) {
        boost::add_edge(value_vertex_map[CS.getInstruction()],
                        value_vertex_map[Formal], CS.getInstruction(), ptg);
      }
    }
  } else {
    ContainedFunctions.insert(F->getName().str());
    // TODO this function has to check if F's points-to graph is already merged
    // into the 'this' points-to graph. If so, is is not allowed to copy it a
    // second
    // time into 'this' ptg.
    vector<pair<PointsToGraph::vertex_t, PointsToGraph::vertex_t>>
        v_in_g1_u_in_g2;
    for (unsigned i = 0; i < CS.getNumArgOperands(); ++i) {
      auto Formal = getNthFunctionArgument(F, i);
      if (value_vertex_map.count(CS.getArgOperand(i)) &&
          Other.value_vertex_map.count(Formal)) {
        v_in_g1_u_in_g2.push_back(
            make_pair(value_vertex_map[CS.getArgOperand(i)],
                      Other.value_vertex_map[Formal]));
      }
    }

    for (auto Formal : Other.getPointersEscapingThroughReturnsForFunction(F)) {
      if (value_vertex_map.count(CS.getInstruction()) &&
          Other.value_vertex_map.count(Formal)) {
        v_in_g1_u_in_g2.push_back(
            make_pair(value_vertex_map[CS.getInstruction()],
                      Other.value_vertex_map[Formal]));
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
    // for more generic graphs, one can try typedef std::map<vertex_t, vertex_t>
    // IsoMap;
    vector<PointsToGraph::vertex_t> orig2copy_data(
        boost::num_vertices(Other.ptg));
    IsoMap mapV = boost::make_iterator_property_map(
        orig2copy_data.begin(), get(boost::vertex_index, Other.ptg));
    boost::copy_graph(Other.ptg, ptg,
                      boost::orig_to_copy(mapV)); // means g1 += g2
    for (auto &entry : v_in_g1_u_in_g2) {
      PointsToGraph::vertex_t u_in_g1 = mapV[entry.second];
      boost::add_edge(entry.first, u_in_g1, CS.getInstruction(), ptg);
    }
  }
  value_vertex_map.clear();
  vertex_iterator_t vi, vi_end;
  for (boost::tie(vi, vi_end) = boost::vertices(ptg); vi != vi_end; ++vi) {
    value_vertex_map.insert(make_pair(ptg[*vi].value, *vi));
  }
}

unsigned PointsToGraph::getNumOfVertices() { return boost::num_vertices(ptg); }

unsigned PointsToGraph::getNumOfEdges() { return boost::num_edges(ptg); }

} // namespace psr
