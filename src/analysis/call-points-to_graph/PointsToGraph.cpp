/*
 * PointsToGraph.cpp
 *
 *  Created on: 08.02.2017
 *      Author: pdschbrt
 */

#include "PointsToGraph.hh"

void PrintResults(const char* Msg, bool P, const llvm::Value* V1,
                  const llvm::Value* V2, const llvm::Module* M) {
  if (P) {
    std::string o1, o2;
    {
      llvm::raw_string_ostream os1(o1), os2(o2);
      V1->printAsOperand(os1, true, M);
      V2->printAsOperand(os2, true, M);
    }

    if (o2 < o1) std::swap(o1, o2);
    llvm::errs() << "  " << Msg << ":\t" << o1 << ", " << o2 << "\n";
  }
}

void PrintModRefResults(const char* Msg, bool P, llvm::Instruction* I,
                        llvm::Value* Ptr, llvm::Module* M) {
  if (P) {
    llvm::errs() << "  " << Msg << ":  Ptr: ";
    Ptr->printAsOperand(llvm::errs(), true, M);
    llvm::errs() << "\t<->" << *I << '\n';
  }
}

void PrintModRefResults(const char* Msg, bool P, llvm::CallSite CSA,
                        llvm::CallSite CSB, llvm::Module* M) {
  if (P) {
    llvm::errs() << "  " << Msg << ": " << *CSA.getInstruction() << " <-> "
                 << *CSB.getInstruction() << '\n';
  }
}

void PrintLoadStoreResults(const char* Msg, bool P, const llvm::Value* V1,
                           const llvm::Value* V2, const llvm::Module* M) {
  if (P) {
    llvm::errs() << "  " << Msg << ": " << *V1 << " <-> " << *V2 << '\n';
  }
}

PointsToGraph::PointsToGraph(llvm::AAResults& AA, llvm::Function* Fu) : F(*Fu) {
  cout << "analyzing function: " << F.getName().str() << endl;
  bool PrintNoAlias, PrintMayAlias, PrintPartialAlias, PrintMustAlias;
  PrintNoAlias = PrintMayAlias = PrintPartialAlias = PrintMustAlias = 1;
  // ModRef information
  bool PrintNoModRef, PrintMod, PrintRef, PrintModRef;
  PrintNoModRef = PrintMod = PrintRef = PrintModRef = 0;
  const llvm::DataLayout& DL = F.getParent()->getDataLayout();
  llvm::SetVector<llvm::Value*> Pointers;
  llvm::SmallSetVector<llvm::CallSite, 16> CallSites;
  llvm::SetVector<llvm::Value*> Loads;
  llvm::SetVector<llvm::Value*> Stores;

  for (auto& I : F.args())
    if (I.getType()->isPointerTy())  // Add all pointer arguments.
      Pointers.insert(&I);

  for (llvm::inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    if (I->getType()->isPointerTy())  // Add all pointer instructions.
      Pointers.insert(&*I);
    if (llvm::isa<llvm::LoadInst>(&*I)) Loads.insert(&*I);
    if (llvm::isa<llvm::StoreInst>(&*I)) Stores.insert(&*I);
    llvm::Instruction& Inst = *I;
    if (auto CS = llvm::CallSite(&Inst)) {
      llvm::Value* Callee = CS.getCalledValue();
      // Skip actual functions for direct function calls.
      if (!llvm::isa<llvm::Function>(Callee) && isInterestingPointer(Callee))
        Pointers.insert(Callee);
      // Consider formals.
      for (llvm::Use& DataOp : CS.data_ops())
        if (isInterestingPointer(DataOp)) Pointers.insert(DataOp);
      CallSites.insert(CS);
    } else {
      // Consider all operands.
      for (llvm::Instruction::op_iterator OI = Inst.op_begin(),
                                          OE = Inst.op_end();
           OI != OE; ++OI)
        if (isInterestingPointer(*OI)) Pointers.insert(*OI);
    }
  }

  llvm::errs() << "Function: " << F.getName() << ": " << Pointers.size()
               << " pointers, " << CallSites.size() << " call sites\n";

  // make vertices for all pointers
  for (auto pointer : Pointers) {
    value_vertex_map[pointer] = boost::add_vertex(ptg);
    string ir;
    llvm::raw_string_ostream rso(ir);
    pointer->print(rso);
    ptg[value_vertex_map[pointer]].ir = ir;
    ptg[value_vertex_map[pointer]].value = pointer;
  }
  // iterate over the worklist, and run the full (n^2)/2 disambiguations
  for (llvm::SetVector<llvm::Value *>::iterator I1 = Pointers.begin(),
                                                E = Pointers.end();
       I1 != E; ++I1) {
    uint64_t I1Size = llvm::MemoryLocation::UnknownSize;
    llvm::Type* I1ElTy =
        llvm::cast<llvm::PointerType>((*I1)->getType())->getElementType();
    if (I1ElTy->isSized()) I1Size = DL.getTypeStoreSize(I1ElTy);

    for (llvm::SetVector<llvm::Value*>::iterator I2 = Pointers.begin();
         I2 != I1; ++I2) {
      uint64_t I2Size = llvm::MemoryLocation::UnknownSize;
      llvm::Type* I2ElTy =
          llvm::cast<llvm::PointerType>((*I2)->getType())->getElementType();
      if (I2ElTy->isSized()) I2Size = DL.getTypeStoreSize(I2ElTy);
      switch (AA.alias(*I1, I1Size, *I2, I2Size)) {
        case llvm::NoAlias:
          PrintResults("NoAlias", PrintNoAlias, *I1, *I2, F.getParent());
          break;
        case llvm::MayAlias:
          PrintResults("MayAlias", PrintMayAlias, *I1, *I2, F.getParent());
          boost::add_edge(value_vertex_map[*I1], value_vertex_map[*I2], ptg);
          break;
        case llvm::PartialAlias:
          PrintResults("PartialAlias", PrintPartialAlias, *I1, *I2,
                       F.getParent());
          boost::add_edge(value_vertex_map[*I1], value_vertex_map[*I2], ptg);
          break;
        case llvm::MustAlias:
          PrintResults("MustAlias", PrintMustAlias, *I1, *I2, F.getParent());
          boost::add_edge(value_vertex_map[*I1], value_vertex_map[*I2], ptg);
          break;
      }
    }
  }
}

bool PointsToGraph::isInterestingPointer(llvm::Value* V) {
  return V->getType()->isPointerTy() &&
         !llvm::isa<llvm::ConstantPointerNull>(V);
}

bool PointsToGraph::containsValue(llvm::Value* V) {
  pair<vertex_iterator_t, vertex_iterator_t> vp;
  for (vp = boost::vertices(ptg); vp.first != vp.second; ++vp.first)
    if (ptg[*vp.first].value == V) return true;
  return false;
}

set<const llvm::Value*> PointsToGraph::isAliasingWithFormals(
    const llvm::Value* V) {
  set<vertex_t> formals;
  for (auto& formal : F.args()) {
    formals.insert(value_vertex_map[&formal]);
  }
  set<vertex_t> aliasing_with_formals;
  formals_reachability_dfs_visitor vis(formals, aliasing_with_formals);
  std::vector<boost::default_color_type> color_map(boost::num_vertices(ptg));
  boost::depth_first_visit(
      ptg, value_vertex_map[V], vis,
      boost::make_iterator_property_map(color_map.begin(),
                                        boost::get(boost::vertex_index, ptg),
                                        color_map[0]));
  set<const llvm::Value*> result;
  for (auto vertex : aliasing_with_formals) {
    result.insert(ptg[vertex].value);
  }
  return result;
}

set<const llvm::Value*> PointsToGraph::getPointsToSet(const llvm::Value* V) {
  set<vertex_t> reachable_vertices;
  reachability_dfs_visitor vis(reachable_vertices);
  vector<boost::default_color_type> color_map(boost::num_vertices(ptg));
  boost::depth_first_visit(
      ptg, value_vertex_map[V], vis,
      boost::make_iterator_property_map(color_map.begin(),
                                        boost::get(boost::vertex_index, ptg),
                                        color_map[0]));
  set<const llvm::Value*> result;
  for (auto vertex : reachable_vertices) {
    result.insert(ptg[vertex].value);
  }
  return result;
}

void PointsToGraph::print() {
  cout << "PointsToGraph for " << F.getName().str() << "\n";
  boost::print_graph(ptg,
                     boost::get(&PointsToGraph::VertexProperties::ir, ptg));
}

void PointsToGraph::merge_graphs(PointsToGraph& g, const llvm::Value* v_in_g,
                                 const PointsToGraph& h,
                                 const llvm::Value* u_in_h) {
  //    typedef boost::property_map<graph_t, boost::vertex_index_t>::type
  //    index_map_t;
  //
  //    //for simple adjacency_list<> this type would be more efficient:
  //    typedef boost::iterator_property_map<typename
  //    std::vector<vertex_t>::iterator, index_map_t,vertex_t,vertex_t&>
  //    IsoMap;
  //
  //    //for more generic graphs, one can try  //typedef std::map<vertex_t,
  //    vertex_t> IsoMap;
  //    vector<vertex_t> orig2copy_data(boost::num_vertices(h.ptg));
  //    IsoMap mapV =
  //    boost::make_iterator_property_map(orig2copy_data.begin(),
  //    get(boost::vertex_index, h.ptg));
  //    boost::copy_graph(g.ptg, h.ptg, boost::orig_to_copy(mapV) ); //means
  //    g1
  //    += g2
  //
  //    vertex_t u_in_g = mapV[value_vertex_map[u_in_h]];
  //    boost::add_edge(value_vertex_map[v_in_g], u_in_g, g.ptg);
}

void PointsToGraph::printValueVertexMap() {
  for (const auto& entry : value_vertex_map) {
    cout << entry.first << " <---> " << entry.second << endl;
  }
}
