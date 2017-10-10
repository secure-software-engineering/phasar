/*
 * LLVMBasedInterproceduralICFG.cpp
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#include "LLVMBasedICFG.hh"

const map<string, WalkerStrategy> StringToWalkerStrategy = {
    {"Simple", WalkerStrategy::Simple},
    {"VariableType", WalkerStrategy::VariableType},
    {"DeclaredType", WalkerStrategy::DeclaredType},
    {"Pointer", WalkerStrategy::Pointer}};

const map<WalkerStrategy, string> WalkerStrategyToString = {
    {WalkerStrategy::Simple, "Simple"},
    {WalkerStrategy::VariableType, "VariableType"},
    {WalkerStrategy::DeclaredType, "DeclaredType"},
    {WalkerStrategy::Pointer, "Pointer"}};

ostream& operator<<(ostream& os, const WalkerStrategy W) {
  return os << WalkerStrategyToString.at(W);
}

const map<string, ResolveStrategy> StringToResolveStrategy = {
    {"CHA", ResolveStrategy::CHA},
    {"RTA", ResolveStrategy::RTA},
    {"TA", ResolveStrategy::TA},
    {"OTF", ResolveStrategy::OTF}};

const map<ResolveStrategy, string> ResolveStrategyToString = {
    {ResolveStrategy::CHA, "CHA"},
    {ResolveStrategy::RTA, "RTA"},
    {ResolveStrategy::TA, "TA"},
    {ResolveStrategy::OTF, "OTF"}};

ostream& operator<<(ostream& os, const ResolveStrategy R) {
  return os << ResolveStrategyToString.at(R);
}

LLVMBasedICFG::VertexProperties::VertexProperties(const llvm::Function* f,
                                                  bool isDecl)
    : function(f), functionName(f->getName().str()), isDeclaration(isDecl) {}

LLVMBasedICFG::EdgeProperties::EdgeProperties(const llvm::Instruction* i)
    : callsite(i),
      ir_code(llvmIRToString(i)),
      id(stoull(llvm::cast<llvm::MDString>(
                    i->getMetadata(MetaDataKind)->getOperand(0))
                    ->getString()
                    .str())) {}

LLVMBasedICFG::LLVMBasedICFG(LLVMStructTypeHierarchy& STH,
                             ProjectIRCompiledDB& IRDB, WalkerStrategy WS,
                             ResolveStrategy RS,
                             const vector<string>& EntryPoints)
    : W(WS), R(RS), CH(STH), IRDB(IRDB) {
  auto& lg = lg::get();
  BOOST_LOG_SEV(lg, INFO) << "Starting call graph construction using "
                             "WalkerStrategy: "
                          << W << " and "
                                  "ResolveStragegy: "
                          << R;
  for (auto& EntryPoint : EntryPoints) {
    llvm::Function* F = IRDB.getFunction(EntryPoint);
    if (F == nullptr)
      throw ios_base::failure(
          "Could not retrieve llvm::Function for entry point");
    PointsToGraph& ptg = *IRDB.getPointsToGraph(EntryPoint);
    WholeModulePTG.mergeWith(ptg, F);
    Walker.at(W)(this, F);
  }
  BOOST_LOG_SEV(lg, INFO) << "Call graph has been constructed";
}

/*
 * Using a lambda to just pass this call to the other constructor.
 */
LLVMBasedICFG::LLVMBasedICFG(LLVMStructTypeHierarchy& STH,
                             ProjectIRCompiledDB& IRDB, const llvm::Module& M,
                             WalkerStrategy WS, ResolveStrategy RS,
                             vector<string> EntryPoints)
    : W(WS), R(RS), CH(STH), IRDB(IRDB) {
  auto& lg = lg::get();
  BOOST_LOG_SEV(lg, INFO) << "Starting call graph construction using "
                             "WalkerStrategy: "
                          << W << " and "
                                  "ResolveStragegy: "
                          << R;
  if (EntryPoints.empty()) {
    for (auto& F : M) {
      EntryPoints.push_back(F.getName().str());
    }
  }
  for (auto& EntryPoint : EntryPoints) {
    llvm::Function* F = M.getFunction(EntryPoint);
    if (F && !F->isDeclaration()) {
      PointsToGraph& ptg = *IRDB.getPointsToGraph(EntryPoint);
      WholeModulePTG.mergeWith(ptg, F);
      Walker.at(W)(this, F);
    }
  }
  BOOST_LOG_SEV(lg, INFO) << "Call graph has been constructed";
}

void LLVMBasedICFG::resolveIndirectCallWalkerSimple(const llvm::Function* F) {
  auto& lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "Walking in function: " << F->getName().str();
  // do not analyze functions more than once (this also acts as recursion
  // detection)
  if (VisitedFunctions.count(F) || F->isDeclaration()) {
    return;
  }
  VisitedFunctions.insert(F);
  // add a node for function F to the call graph (if not present already)
  if (!function_vertex_map.count(F->getName().str())) {
    function_vertex_map[F->getName().str()] = boost::add_vertex(cg);
    cg[function_vertex_map[F->getName().str()]] = VertexProperties(F);
  }
  for (llvm::const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E;
       ++I) {
    const llvm::Instruction& Inst = *I;
    if (llvm::isa<llvm::CallInst>(Inst) || llvm::isa<llvm::InvokeInst>(Inst)) {
      CallStack.push_back(&Inst);
      llvm::ImmutableCallSite cs(&Inst);
      set<const llvm::Function*> possible_targets;
      // check if function call can be resolved statically
      if (cs.getCalledFunction() != nullptr) {
        possible_targets.insert(cs.getCalledFunction());
        BOOST_LOG_SEV(lg, DEBUG) << "Found static call-site: "
                                 << llvmIRToString(cs.getInstruction());
      } else {  // the function call must be resolved dynamically
        BOOST_LOG_SEV(lg, DEBUG) << "Found dynamic call-site: "
                                 << llvmIRToString(cs.getInstruction());
        // call the resolve routine
        set<string> possible_target_names = Resolver.at(R)(this, cs);
        for (auto& possible_target_name : possible_target_names) {
          possible_targets.insert(IRDB.getFunction(possible_target_name));
        }
      }
      BOOST_LOG_SEV(lg, DEBUG) << "Found " << possible_targets.size()
                               << " possible target(s)";
      for (auto possible_target : possible_targets) {
        if (!function_vertex_map.count(possible_target->getName().str())) {
          function_vertex_map[possible_target->getName().str()] =
              boost::add_vertex(cg);
          cg[function_vertex_map[possible_target->getName().str()]] =
              VertexProperties(possible_target,
                               possible_target->isDeclaration());
        }
        boost::add_edge(function_vertex_map[F->getName().str()],
                        function_vertex_map[possible_target->getName().str()],
                        EdgeProperties(cs.getInstruction()), cg);
      }
      // continue resolving
      for (auto possible_target : possible_targets) {
        resolveIndirectCallWalkerSimple(possible_target);
      }
      CallStack.pop_back();
    }
  }
}

void LLVMBasedICFG::resolveIndirectCallWalkerTypeAnalysis(
    const llvm::Function* F,
    function<set<string>(llvm::ImmutableCallSite CS)> R, bool useVTA) {
  throw runtime_error("Not implemented yet!");
}

void LLVMBasedICFG::resolveIndirectCallWalkerPointerAnalysis(
    const llvm::Function* F) {
  auto& lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "Walking in function: " << F->getName().str();
  // do not analyze functions more than once (this also acts as recursion
  // detection)
  if (VisitedFunctions.count(F) || F->isDeclaration()) {
    BOOST_LOG_SEV(lg, DEBUG) << "Function alredy visited or only declaration: "
                             << F->getName().str();
    return;
  }
  VisitedFunctions.insert(F);
  // add a node for function F to the call graph (if not present already)
  if (!function_vertex_map.count(F->getName().str())) {
    function_vertex_map[F->getName().str()] = boost::add_vertex(cg);
    cg[function_vertex_map[F->getName().str()]] =
        VertexProperties(F, F->isDeclaration());
  }
  for (llvm::const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E;
       ++I) {
    const llvm::Instruction& Inst = *I;
    if (llvm::isa<llvm::CallInst>(Inst) || llvm::isa<llvm::InvokeInst>(Inst)) {
      CallStack.push_back(&Inst);
      llvm::ImmutableCallSite cs(&Inst);
      set<const llvm::Function*> possible_targets;
      // function can be resolved statically
      if (cs.getCalledFunction() != nullptr) {
        BOOST_LOG_SEV(lg, DEBUG) << "Found static call-site: "
                                 << llvmIRToString(cs.getInstruction());
        possible_targets.insert(cs.getCalledFunction());
      } else {
        // we have to resolve the called function ourselves using the accessible
        // points-to information
        BOOST_LOG_SEV(lg, DEBUG) << "Found dynamic call-site: "
                                 << llvmIRToString(cs.getInstruction());
        // call the resolve routine
        set<string> possible_target_names = Resolver.at(R)(this, cs);
        for (auto& possible_target_name : possible_target_names) {
          possible_targets.insert(IRDB.getFunction(possible_target_name));
        }
      }
      BOOST_LOG_SEV(lg, DEBUG) << "Found " << possible_targets.size()
                               << " possible target(s)";
      for (auto possible_target : possible_targets) {
        // Do the merge of the points-to graphs for all possible targets, but
        // only if they are available
        if (!F->getParent()
                 ->getFunction(possible_target->getName().str())
                 ->isDeclaration()) {
          PointsToGraph& callee_ptg =
              *IRDB.getPointsToGraph(possible_target->getName().str());
          WholeModulePTG.mergeWith(callee_ptg, cs, possible_target);
        }
        // add into boost graph
        if (!function_vertex_map.count(possible_target->getName().str())) {
          function_vertex_map[possible_target->getName().str()] =
              boost::add_vertex(cg);
          cg[function_vertex_map[possible_target->getName().str()]] =
              VertexProperties(possible_target,
                               possible_target->isDeclaration());
        }
        boost::add_edge(function_vertex_map[F->getName().str()],
                        function_vertex_map[possible_target->getName().str()],
                        EdgeProperties(cs.getInstruction()), cg);
      }
      // continue resolving recursively
      for (auto possible_target : possible_targets) {
        resolveIndirectCallWalkerPointerAnalysis(possible_target);
      }
      CallStack.pop_back();
    }
  }
}

set<string> LLVMBasedICFG::resolveIndirectCallOTF(llvm::ImmutableCallSite CS) {
  auto& lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "Resolve call";
  set<string> possible_call_targets;
  // check if we have a virtual call-site
  if (CS.getNumArgOperands() > 0) {
    // deal with a virtual member function
    // retrieve the vtable entry that is called
    const llvm::LoadInst* load =
        llvm::dyn_cast<llvm::LoadInst>(CS.getCalledValue());
    const llvm::GetElementPtrInst* gep =
        llvm::dyn_cast<llvm::GetElementPtrInst>(load->getPointerOperand());
    unsigned vtable_index =
        llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(1))->getZExtValue();
    BOOST_LOG_SEV(lg, DEBUG) << "Virtual function table entry is: "
                             << vtable_index;
    const llvm::Value* receiver = CS.getArgOperand(0);
    const llvm::Type* receiver_type = receiver->getType();
    auto alloc_sites =
        WholeModulePTG.getReachableAllocationSites(receiver, CallStack);
    auto possible_types =
        WholeModulePTG.computeTypesFromAllocationSites(alloc_sites);
    if (possible_types.empty()) {
      BOOST_LOG_SEV(lg, WARNING)
          << "Could not find any possible type, using fall-back solution";
      // here we take the conservative fall-back solution
      if (const llvm::StructType* struct_type =
              llvm::dyn_cast<llvm::StructType>(receiver_type)) {
        // insert declared type
        // here we have to call debasify() since the following IR might be
        // generated:
        //   %struct.A = type <{ i32 (...)**, i32, [4 x i8] }>
        //   %struct.B = type { %struct.A.base, [4 x i8] }
        //   %struct.A.base = type <{ i32 (...)**, i32 }>
        // in such a case %struct.A and %struct.A.base are treated as aliases
        // and the vtable for A is stored
        // under the name %struct.A whereas in the class hierarchy
        // %struct.A.base is used!
        // debasify() fixes that circumstance and returns it for all normal
        // cases.
        possible_call_targets.insert(CH.getVTableEntry(
            debasify(struct_type->getName().str()), vtable_index));
        // also insert all possible subtypes
        auto fallback_type_names =
            CH.getTransitivelyReachableTypes(struct_type->getName().str());
        for (auto& fallback_name : fallback_type_names) {
          possible_call_targets.insert(
              CH.getVTableEntry(fallback_name, vtable_index));
        }
      }
    } else {
      cout << "found the following possible types" << endl;
      for (auto type : possible_types) {
        type->dump();
        // caution: type might be a pointer type returned by a allocating
        // function
        const llvm::StructType* struct_type =
            (!type->isPointerTy()) ? llvm::dyn_cast<llvm::StructType>(type)
                                   : llvm::dyn_cast<llvm::StructType>(
                                         type->getPointerElementType());
        if (struct_type) {
          // same as above
          possible_call_targets.insert(CH.getVTableEntry(
              debasify(struct_type->getName().str()), vtable_index));
        }
      }
    }
    cout << "possible targets are:" << endl;
    for (auto entry : possible_call_targets) {
      cout << entry << endl;
    }

  } else {  // else we have to deal with a function pointer
    cout << "function pointer" << endl;
    if (CS.getCalledValue()->getType()->isPointerTy()) {
      if (const llvm::FunctionType* ftype = llvm::dyn_cast<llvm::FunctionType>(
              CS.getCalledValue()->getType())) {
        for (auto entry : IRDB.functions) {
          const llvm::Function* f =
              IRDB.modules[entry.second]->getFunction(entry.first);
          if (matchesSignature(f, ftype)) {
            possible_call_targets.insert(entry.first);
          }
        }
      }
    }
  }
  return possible_call_targets;
}

set<string> LLVMBasedICFG::resolveIndirectCallCHA(llvm::ImmutableCallSite CS) {
  throw runtime_error("Not implemented yet!");
}

set<string> LLVMBasedICFG::resolveIndirectCallRTA(llvm::ImmutableCallSite CS) {
  throw runtime_error("Not implemented yet!");
}

set<string> LLVMBasedICFG::resolveIndirectCallTA(llvm::ImmutableCallSite CS) {
  throw runtime_error("Not implemented yet!");
}

bool LLVMBasedICFG::isMemberFunctionCall(const llvm::Instruction* I) {
  set<const llvm::Function*> Callees = getCalleesOfCallAt(I);
  for (auto F : Callees) {
    if (F->getArgumentList().size()) {
      auto Type = F->getArgumentList().begin()->getType();
      if (Type->isPointerTy()) {
        Type = Type->getPointerElementType();
        if (auto ST = llvm::dyn_cast<llvm::StructType>(Type)) {
          string DemangledName = cxx_demangle(F->getName().str());
          string TypeName = ST->getName().str();
          return CH.containsType(TypeName) &&
                 DemangledName.find(CH.getPlainTypename(TypeName)) !=
                     string::npos;
        }
      }
    }
  }
  return false;
}

const llvm::Function* LLVMBasedICFG::getMethodOf(
    const llvm::Instruction* stmt) {
  return stmt->getFunction();
}

const llvm::Function* LLVMBasedICFG::getMethod(const string& fun) {
  return IRDB.getFunction(fun);
}

vector<const llvm::Instruction*> LLVMBasedICFG::getPredsOf(
    const llvm::Instruction* I) {
  vector<const llvm::Instruction*> Preds;
  if (I->getPrevNode()) {
    Preds.push_back(I->getPrevNode());
  }
  /*
   * If we do not have a predecessor yet, look for basic blocks which
   * lead to our instruction in question!
   */
  if (Preds.empty()) {
    for (auto& BB : *I->getFunction()) {
      if (const llvm::TerminatorInst* T =
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

vector<const llvm::Instruction*> LLVMBasedICFG::getSuccsOf(
    const llvm::Instruction* I) {
  vector<const llvm::Instruction*> Successors;
  if (I->getNextNode()) Successors.push_back(I->getNextNode());
  if (const llvm::TerminatorInst* T = llvm::dyn_cast<llvm::TerminatorInst>(I)) {
    for (auto successor : T->successors()) {
      Successors.push_back(&*successor->begin());
    }
  }
  return Successors;
}

vector<pair<const llvm::Instruction*, const llvm::Instruction*>>
LLVMBasedICFG::getAllControlFlowEdges(const llvm::Function* fun) {
  vector<pair<const llvm::Instruction*, const llvm::Instruction*>> Edges;
  for (auto& BB : *fun) {
    for (auto& I : BB) {
      auto Successors = getSuccsOf(&I);
      for (auto Successor : Successors) {
        Edges.push_back(make_pair(&I, Successor));
      }
    }
  }
  return Edges;
}

vector<const llvm::Instruction*> LLVMBasedICFG::getAllInstructionsOf(
    const llvm::Function* fun) {
  vector<const llvm::Instruction*> Instructions;
  for (auto& BB : *fun) {
    for (auto& I : BB) {
      Instructions.push_back(&I);
    }
  }
  return Instructions;
}

bool LLVMBasedICFG::isExitStmt(const llvm::Instruction* stmt) {
  return llvm::isa<llvm::ReturnInst>(stmt);
}

bool LLVMBasedICFG::isStartPoint(const llvm::Instruction* stmt) {
  return (stmt == &stmt->getFunction()->front().front());
}

bool LLVMBasedICFG::isFallThroughSuccessor(const llvm::Instruction* stmt,
                                           const llvm::Instruction* succ) {
  if (const llvm::BranchInst* B = llvm::dyn_cast<llvm::BranchInst>(succ)) {
    if (B->isConditional()) {
      return &B->getSuccessor(1)->front() == succ;
    } else {
      return &B->getSuccessor(0)->front() == succ;
    }
  }
  return false;
}

bool LLVMBasedICFG::isBranchTarget(const llvm::Instruction* stmt,
                                   const llvm::Instruction* succ) {
  if (const llvm::TerminatorInst* T =
          llvm::dyn_cast<llvm::TerminatorInst>(stmt)) {
    for (auto successor : T->successors()) {
      if (&*successor->begin() == succ) {
        return true;
      }
    }
  }
  return false;
}

string LLVMBasedICFG::getMethodName(const llvm::Function* fun) {
  return fun->getName().str();
}

/**
 * Returns all callee methods for a given call that might be called.
 */
set<const llvm::Function*> LLVMBasedICFG::getCalleesOfCallAt(
    const llvm::Instruction* n) {
  if (llvm::isa<llvm::CallInst>(n) || llvm::isa<llvm::InvokeInst>(n)) {
    llvm::ImmutableCallSite CS(n);
    // handle direct call
    if (CS.getCalledFunction()) {
      return {CS.getCalledFunction()};
    } else {  // handle indirect call
      set<const llvm::Function*> Callees;
      string CallerName = CS->getFunction()->getName().str();
      out_edge_iterator ei, ei_end;
      for (boost::tie(ei, ei_end) =
               boost::out_edges(function_vertex_map[CallerName], cg);
           ei != ei_end; ++ei) {
        auto source = boost::source(*ei, cg);
        auto edge = cg[*ei];
        auto target = boost::target(*ei, cg);
        if (CS.getInstruction() == edge.callsite) {
          Callees.insert(cg[target].function);
        }
      }
      return Callees;
    }
  } else {
    // neither call nor invoke - error!
    llvm::errs() << "ERROR: found instruction that is neither CallInst nor "
                    "InvokeInst\n";
    HEREANDNOW;
    DIE_HARD;
    return {};
  }
}

/**
 * Returns all caller statements/nodes of a given method.
 */
set<const llvm::Instruction*> LLVMBasedICFG::getCallersOf(
    const llvm::Function* m) {
  set<const llvm::Instruction*> CallersOf;
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
set<const llvm::Instruction*> LLVMBasedICFG::getCallsFromWithin(
    const llvm::Function* f) {
  set<const llvm::Instruction*> CallSites;
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
set<const llvm::Instruction*> LLVMBasedICFG::getStartPointsOf(
    const llvm::Function* m) {
  if (!m->isDeclaration()) {
    return {&m->front().front()};
  } else {
    auto& lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG) << "Could not get starting points of '"
                             << m->getName().str()
                             << "' because it is a declaration!";
    return {};
  }
}

set<const llvm::Instruction*> LLVMBasedICFG::getExitPointsOf(
    const llvm::Function* fun) {
  if (!fun->isDeclaration()) {
    return {&fun->back().back()};
  } else {
    auto& lg = lg::get();
    BOOST_LOG_SEV(lg, DEBUG) << "Could not get exit points of '"
                             << fun->getName().str()
                             << "' which is declaration!";
    return {};
  }
}

/**
 * Returns all statements to which a call could return.
 * In the RHS paper, for every call there is just one return site.
 * We, however, use as return site the successor statements, of which
 * there can be many in case of exceptional flow.
 */
set<const llvm::Instruction*> LLVMBasedICFG::getReturnSitesOfCallAt(
    const llvm::Instruction* n) {
  // at the moment we just ignore exceptional control flow
  set<const llvm::Instruction*> ReturnSites;
  if (llvm::isa<llvm::CallInst>(n) || llvm::isa<llvm::InvokeInst>(n)) {
    // ReturnSites.insert(n);
    llvm::ImmutableCallSite CS(n);
    for (auto user : CS->users()) {
      if (const llvm::Instruction* inst =
              llvm::dyn_cast<llvm::Instruction>(user)) {
        ReturnSites.insert(inst);
      }
    }
    if (ReturnSites.empty()) {
      ReturnSites.insert(n->getNextNode());
    }
  }
  return ReturnSites;
}

bool LLVMBasedICFG::isCallStmt(const llvm::Instruction* stmt) {
  if (llvm::isa<llvm::CallInst>(stmt) || llvm::isa<llvm::InvokeInst>(stmt)) {
    return true;
  } else {
    return false;
  }
}

/**
 * Returns the set of all nodes that are neither call nor start nodes.
 */
set<const llvm::Instruction*> LLVMBasedICFG::allNonCallStartNodes() {
  set<const llvm::Instruction*> NonCallStartNodes;
  for (auto M : IRDB.getAllModules()) {
    for (auto& F : *M) {
      for (auto& BB : F) {
        for (auto& I : BB) {
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

vector<const llvm::Instruction*> LLVMBasedICFG::getAllInstructionsOfFunction(
    const string& name) {
  return getAllInstructionsOf(IRDB.getFunction(name));
}

const llvm::Instruction* LLVMBasedICFG::getLastInstructionOf(
    const string& name) {
  const llvm::Function& f = *IRDB.getFunction(name);
  auto last = llvm::inst_end(f);
  last--;
  return &(*last);
}

void LLVMBasedICFG::mergeWith(const LLVMBasedICFG& other) {
  // Copy other graph into this graph
  typedef typename boost::property_map<bidigraph_t, boost::vertex_index_t>::type
      index_map_t;
  // For simple adjacency_list<> this type would be more efficient:
  typedef typename boost::iterator_property_map<
      typename std::vector<LLVMBasedICFG::vertex_t>::iterator, index_map_t,
      LLVMBasedICFG::vertex_t, LLVMBasedICFG::vertex_t&>
      IsoMap;
  // For more generic graphs, one can try typedef std::map<vertex_t, vertex_t>
  // IsoMap;
  vector<LLVMBasedICFG::vertex_t> orig2copy_data(boost::num_vertices(other.cg));
  IsoMap mapV = boost::make_iterator_property_map(
      orig2copy_data.begin(), get(boost::vertex_index, other.cg));
  boost::copy_graph(other.cg, cg, boost::orig_to_copy(mapV));  // means g1 += g2
  // This vector hols the call-sites that are used to merge the whole-module
  // points-to graphs
  vector<pair<llvm::ImmutableCallSite, const llvm::Function*>> Calls;
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

bool LLVMBasedICFG::isPrimitiveFunction(const string& name) {
  for (auto& BB : *IRDB.getFunction(name)) {
    for (auto& I : BB) {
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

void LLVMBasedICFG::printAsDot(const string& filename) {
  ofstream ofs(filename);
  boost::write_graphviz(
      ofs, cg, boost::make_label_writer(boost::get(
                   &LLVMBasedICFG::VertexProperties::functionName, cg)),
      boost::make_label_writer(
          boost::get(&LLVMBasedICFG::EdgeProperties::ir_code, cg)));
}

void LLVMBasedICFG::printInternalPTGAsDot(const string& filename) {
  ofstream ofs(filename);
  boost::write_graphviz(
      ofs, WholeModulePTG.ptg,
      boost::make_label_writer(boost::get(
          &PointsToGraph::VertexProperties::ir_code, WholeModulePTG.ptg)),
      boost::make_label_writer(boost::get(
          &PointsToGraph::EdgeProperties::ir_code, WholeModulePTG.ptg)));
}

void LLVMBasedICFG::exportPATBCJSON() {
  cout << "LLVMBasedICFG::exportPATBCJSON()\n";
}

PointsToGraph& LLVMBasedICFG::getWholeModulePTG() { return WholeModulePTG; }

vector<string> LLVMBasedICFG::getDependencyOrderedFunctions() {
  vector<vertex_t> vertices;
  vector<string> functionNames;
  dependency_visitor deps(vertices);
  boost::depth_first_search(cg, visitor(deps));
  for (auto v : vertices) {
    functionNames.push_back(cg[v].functionName);
  }
  return functionNames;
}
