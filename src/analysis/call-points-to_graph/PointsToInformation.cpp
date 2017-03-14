/*
 * PointsToGraph.cpp
 *
 *  Created on: 08.02.2017
 *      Author: pdschbrt
 */

#include "PointsToInformation.hh"

void PrintResults(const char *Msg, bool P, const llvm::Value *V1,
                  const llvm::Value *V2, const llvm::Module *M) {
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

PointsToInformation::PointsToInformation(AliasKind cons)
    : consideration(cons) {}

void PointsToInformation::analyzeModule(llvm::AAResults &AA, llvm::Module &M) {
  // see http://llvm.org/docs/AliasAnalysis.html
  // Alias information
  bool PrintNoAlias, PrintMayAlias, PrintPartialAlias, PrintMustAlias;
  PrintNoAlias = PrintMayAlias = PrintPartialAlias = PrintMustAlias = 1;
  // ModRef information
  bool PrintNoModRef, PrintMod, PrintRef, PrintModRef;
  PrintNoModRef = PrintMod = PrintRef = PrintModRef = 0;
  const llvm::DataLayout &DL = M.getDataLayout();
  for (llvm::Function &F : M.functions()) {
    string Fname = F.getName().str();
    cout << "analyzing function: " << F.getName().str() << endl;
    llvm::SetVector<llvm::Value *> Pointers;
    llvm::SmallSetVector<llvm::CallSite, 16> CallSites;
    llvm::SetVector<llvm::Value *> Loads;
    llvm::SetVector<llvm::Value *> Stores;

    for (auto &I : F.args())
      if (I.getType()->isPointerTy())  // Add all pointer arguments.
        Pointers.insert(&I);

    for (llvm::inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (I->getType()->isPointerTy())  // Add all pointer instructions.
        Pointers.insert(&*I);
      if (llvm::isa<llvm::LoadInst>(&*I)) Loads.insert(&*I);
      if (llvm::isa<llvm::StoreInst>(&*I)) Stores.insert(&*I);
      llvm::Instruction &Inst = *I;
      if (auto CS = llvm::CallSite(&Inst)) {
        llvm::Value *Callee = CS.getCalledValue();
        // Skip actual functions for direct function calls.
        if (!llvm::isa<llvm::Function>(Callee) && isInterestingPointer(Callee))
          Pointers.insert(Callee);
        // Consider formals.
        for (llvm::Use &DataOp : CS.data_ops())
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
      value_vertex_map[pointer] = boost::add_vertex(function_ptg_map[Fname]);
      string ir;
      llvm::raw_string_ostream rso(ir);
      pointer->print(rso);
      function_ptg_map[Fname][value_vertex_map[pointer]].ir = ir;
      function_ptg_map[Fname][value_vertex_map[pointer]].value = pointer;
    }

    // iterate over the worklist, and run the full (n^2)/2 disambiguations
    for (llvm::SetVector<llvm::Value *>::iterator I1 = Pointers.begin(),
                                                  E = Pointers.end();
         I1 != E; ++I1) {
      uint64_t I1Size = llvm::MemoryLocation::UnknownSize;
      llvm::Type *I1ElTy =
          llvm::cast<llvm::PointerType>((*I1)->getType())->getElementType();
      if (I1ElTy->isSized()) I1Size = DL.getTypeStoreSize(I1ElTy);

      for (llvm::SetVector<llvm::Value *>::iterator I2 = Pointers.begin();
           I2 != I1; ++I2) {
        uint64_t I2Size = llvm::MemoryLocation::UnknownSize;
        llvm::Type *I2ElTy =
            llvm::cast<llvm::PointerType>((*I2)->getType())->getElementType();
        if (I2ElTy->isSized()) I2Size = DL.getTypeStoreSize(I2ElTy);

        switch (AA.alias(*I1, I1Size, *I2, I2Size)) {
          case llvm::NoAlias:
            // PrintResults("NoAlias", PrintNoAlias, *I1, *I2, F.getParent());
            break;
          case llvm::MayAlias:
            // PrintResults("MayAlias", PrintMayAlias, *I1, *I2, F.getParent());
            // boost::add_edge(value_vertex_map[*I1], value_vertex_map[*I2],
            // function_ptg_map[Fname]);
            break;
          case llvm::PartialAlias:
            // PrintResults("PartialAlias", PrintPartialAlias, *I1, *I2,
            // F.getParent());
            // cout << "found partial alias" << endl;
            boost::add_edge(value_vertex_map[*I1], value_vertex_map[*I2],
                            function_ptg_map[Fname]);
            break;
          case llvm::MustAlias:
            // PrintResults("MustAlias", PrintMustAlias, *I1, *I2,
            // F.getParent());
            boost::add_edge(value_vertex_map[*I1], value_vertex_map[*I2],
                            function_ptg_map[Fname]);
            break;
        }
      }

      //			// iterate over all pairs of load, store
      //			for (llvm::SetVector<llvm::Value *>::iterator I1
      //= Loads.begin(),
      //					E = Loads.end(); I1 != E; ++I1)
      //{
      //				for (llvm::SetVector<llvm::Value
      //*>::iterator I2 =
      //						Stores.begin(), E2 =
      //Stores.end(); I2 != E2; ++I2) {
      //					switch (AA.alias(
      //							llvm::MemoryLocation::get(
      //									llvm::cast<llvm::LoadInst>(*I1)),
      //							llvm::MemoryLocation::get(
      //									llvm::cast<llvm::StoreInst>(*I2))))
      //{
      //					case llvm::NoAlias:
      //						PrintLoadStoreResults("NoAlias",
      //PrintNoAlias, *I1, *I2,
      //								F.getParent());
      //						break;
      //					case llvm::MayAlias:
      //						PrintLoadStoreResults("MayAlias",
      //PrintMayAlias, *I1,
      //								*I2,
      //F.getParent());
      //						break;
      //					case llvm::PartialAlias:
      //						PrintLoadStoreResults("PartialAlias",
      //PrintPartialAlias,
      //								*I1,
      //*I2, F.getParent());
      //						break;
      //					case llvm::MustAlias:
      //						PrintLoadStoreResults("MustAlias",
      //PrintMustAlias, *I1,
      //								*I2,
      //F.getParent());
      //						break;
      //					}
      //				}
      //			}
      //
      //			// iterate over all pairs of store, store
      //			for (llvm::SetVector<llvm::Value *>::iterator I1
      //= Stores.begin(),
      //					E = Stores.end(); I1 != E; ++I1)
      //{
      //				for (llvm::SetVector<llvm::Value
      //*>::iterator I2 =
      //						Stores.begin(); I2 !=
      //I1; ++I2) {
      //					switch (AA.alias(
      //							llvm::MemoryLocation::get(
      //									llvm::cast<llvm::StoreInst>(*I1)),
      //							llvm::MemoryLocation::get(
      //									llvm::cast<llvm::StoreInst>(*I2))))
      //{
      //					case llvm::NoAlias:
      //						PrintLoadStoreResults("NoAlias",
      //PrintNoAlias, *I1, *I2,
      //								F.getParent());
      //						break;
      //					case llvm::MayAlias:
      //						PrintLoadStoreResults("MayAlias",
      //PrintMayAlias, *I1,
      //								*I2,
      //F.getParent());
      //						break;
      //					case llvm::PartialAlias:
      //						PrintLoadStoreResults("PartialAlias",
      //PrintPartialAlias,
      //								*I1,
      //*I2, F.getParent());
      //						break;
      //					case llvm::MustAlias:
      //						PrintLoadStoreResults("MustAlias",
      //PrintMustAlias, *I1,
      //								*I2,
      //F.getParent());
      //						break;
      //					}
      //				}
      //			}
      //
      //			// Mod/ref alias analysis: compare all pairs of
      //calls and values
      //			for (auto C = CallSites.begin(), Ce =
      //CallSites.end(); C != Ce;
      //					++C) {
      //				llvm::Instruction *I =
      //C->getInstruction();
      //
      //				for (llvm::SetVector<llvm::Value
      //*>::iterator V =
      //						Pointers.begin(), Ve =
      //Pointers.end(); V != Ve; ++V) {
      //					uint64_t Size =
      //llvm::MemoryLocation::UnknownSize;
      //					llvm::Type *ElTy =
      //llvm::cast<llvm::PointerType>(
      //							(*V)->getType())->getElementType();
      //					if (ElTy->isSized())
      //						Size =
      //DL.getTypeStoreSize(ElTy);
      //
      //					switch (AA.getModRefInfo(*C, *V,
      //Size)) {
      //					case llvm::MRI_NoModRef:
      //						PrintModRefResults("NoModRef",
      //PrintNoModRef, I, *V,
      //								F.getParent());
      //						break;
      //					case llvm::MRI_Mod:
      //						PrintModRefResults("Just
      //Mod", PrintMod, I, *V,
      //								F.getParent());
      //						break;
      //					case llvm::MRI_Ref:
      //						PrintModRefResults("Just
      //Ref", PrintRef, I, *V,
      //								F.getParent());
      //						break;
      //					case llvm::MRI_ModRef:
      //						PrintModRefResults("Both
      //ModRef", PrintModRef, I, *V,
      //								F.getParent());
      //						break;
      //					}
      //				}
      //			}
      //
      //			// Mod/ref alias analysis: compare all pairs of
      //calls
      //			for (auto C = CallSites.begin(), Ce =
      //CallSites.end(); C != Ce;
      //					++C) {
      //				for (auto D = CallSites.begin(); D !=
      //Ce; ++D) {
      //					if (D == C)
      //						continue;
      //					switch (AA.getModRefInfo(*C,
      //*D)) {
      //					case llvm::MRI_NoModRef:
      //						PrintModRefResults("NoModRef",
      //PrintNoModRef, *C, *D,
      //								F.getParent());
      //						break;
      //					case llvm::MRI_Mod:
      //						PrintModRefResults("Just
      //Mod", PrintMod, *C, *D,
      //								F.getParent());
      //						break;
      //					case llvm::MRI_Ref:
      //						PrintModRefResults("Just
      //Ref", PrintRef, *C, *D,
      //								F.getParent());
      //						break;
      //					case llvm::MRI_ModRef:
      //						PrintModRefResults("Both
      //ModRef", PrintModRef, *C, *D,
      //								F.getParent());
      //						break;
      //					}
      //				}
      //			}
    }
  }
  //	ofstream out("ptg_main.dot");
  //	boost::write_graphviz(out,
  //function_ptg_map[M.getFunction("main")->getName().str()]);
}

bool PointsToInformation::isInterestingPointer(llvm::Value *V) {
  return V->getType()->isPointerTy() &&
         !llvm::isa<llvm::ConstantPointerNull>(V);
}

set<const llvm::Value *> PointsToInformation::aliasWithFormalParameter(
    const llvm::Function *F, const llvm::Value *V) {
  set<vertex_t> formals;
  for (auto &formal : F->args()) {
    formals.insert(value_vertex_map[&formal]);
  }
  set<vertex_t> aliasing_with_formals;
  formals_reachability_dfs_visitor vis(formals, aliasing_with_formals);
  std::vector<boost::default_color_type> color_map(
      boost::num_vertices(function_ptg_map[F->getName().str()]));
  boost::depth_first_visit(
      function_ptg_map[F->getName().str()], value_vertex_map[V], vis,
      boost::make_iterator_property_map(
          color_map.begin(),
          boost::get(boost::vertex_index, function_ptg_map[F->getName().str()]),
          color_map[0]));
  set<const llvm::Value *> result;
  for (auto vertex : aliasing_with_formals) {
    result.insert(function_ptg_map[F->getName().str()][vertex].value);
  }
  return result;
}

set<const llvm::Value *> PointsToInformation::getPointsToSet(
    const llvm::Function *F, const llvm::Value *V) {
  set<vertex_t> reachable_vertices;
  reachability_dfs_visitor vis(reachable_vertices);
  vector<boost::default_color_type> color_map(
      boost::num_vertices(function_ptg_map[F->getName().str()]));
  boost::depth_first_visit(
      function_ptg_map[F->getName().str()], value_vertex_map[V], vis,
      boost::make_iterator_property_map(
          color_map.begin(),
          boost::get(boost::vertex_index, function_ptg_map[F->getName().str()]),
          color_map[0]));
  set<const llvm::Value *> result;
  for (auto vertex : reachable_vertices) {
    result.insert(function_ptg_map[F->getName().str()][vertex].value);
  }
  return result;
}

ostream &operator<<(ostream &os, const PointsToInformation &ptg) {
  os << "PointsToGraphs:\n";
  for (auto entry : ptg.function_ptg_map) {
    os << "graph for " << entry.first << endl;
    boost::print_graph(
        entry.second,
        boost::get(&PointsToInformation::VertexProperties::ir, entry.second));
  }
  return os;
}

void PointsToInformation::printValueVertexMap() {
  for (auto entry : value_vertex_map) {
    string str;
    llvm::raw_string_ostream rso(str);
    entry.first->print(rso);
    cout << str << " --- " << entry.second << endl;
  }
}

void PointsToInformation::write_pti_graphviz(const string path,
                                             const llvm::Function *F) {
  ofstream out(path);
  boost::write_graphviz(out, function_ptg_map[F->getName().str()]);
}
