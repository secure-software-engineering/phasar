/*
 * LLVMBasedInterproceduralICFG.cpp
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#include "LLVMBasedICFG.hh"

ostream& operator<< (ostream& os, const WalkerStrategy W) {
	static const array<string, 4> str = {{ "Simple", "VariableType", "DeclaredType", "Pointer" }};
	return os << str.at(static_cast<underlying_type_t<WalkerStrategy>>(W));
}

ostream& operator<< (ostream& os, const ResolveStrategy R) {
	static const array<string, 4> str = {{ "CHA", "RTA", "TA", "OTF" }};
	return os << str.at(static_cast<underlying_type_t<ResolveStrategy>>(R));
}

LLVMBasedICFG::VertexProperties::VertexProperties(const llvm::Function* f, bool isDecl) : function(f),
																																						 functionName(f->getName().str()),
																																						 isDeclaration(isDecl) {

}

LLVMBasedICFG::EdgeProperties::EdgeProperties(const llvm::Instruction* i) : callsite(i),
																																						ir_code(llvmIRToString(i)),
																																						id(stoull(llvm::cast<llvm::MDString>(i->getMetadata(MetaDataKind)->getOperand(0))->getString().str())) {

}

LLVMBasedICFG::LLVMBasedICFG(LLVMStructTypeHierarchy& STH,
														 ProjectIRCompiledDB& IRDB,
														 WalkerStrategy W,
                						 ResolveStrategy R,
														 const vector<string>& EntryPoints)
		: CH(STH), IRDB(IRDB), W(W), R(R) {
	auto& lg = lg::get();
	BOOST_LOG_SEV(lg, INFO) << "Starting call graph construction using "
														 "WalkerStrategy: " << W << " and "
														 "ResolveStragegy: " << R;
	for (auto& EntryPoint : EntryPoints) {
			llvm::Function* function = IRDB.getFunction(EntryPoint);
			PointsToGraph& ptg = *IRDB.getPointsToGraph(EntryPoint);
			WholeModulePTG.mergeWith(ptg, {}, nullptr);
			Walker.at(W)(this, function);
	}
	BOOST_LOG_SEV(lg, INFO) << "Call graph has been constructed";
}

/*
 * Using a lambda to just pass this call to the other constructor.
 */
 LLVMBasedICFG::LLVMBasedICFG(LLVMStructTypeHierarchy& STH,
														 ProjectIRCompiledDB& IRDB,
														 const llvm::Module& M,
														 WalkerStrategy W,
                						 ResolveStrategy R)
		: LLVMBasedICFG(STH, IRDB, W, R, [](const llvm::Module& M) {
																			vector<string> result;
																			for (auto& F : M) {
																				result.push_back(F.getName().str());
																			}
																			return result;
		}(M)) {}

void LLVMBasedICFG::resolveIndirectCallWalkerSimple(
     const llvm::Function *F) {
	// do not analyze functions more than once (this also acts as recursion detection)
	if (VisitedFunctions.count(F) || F->isDeclaration()) {
		return;
	}
	VisitedFunctions.insert(F);
	// add a node for function F to the call graph (if not present already)
	if (function_vertex_map.find(F->getName().str()) == function_vertex_map.end()) {
  	function_vertex_map[F->getName().str()] = boost::add_vertex(cg);
  	cg[function_vertex_map[F->getName().str()]] = VertexProperties(F);
  }
  for (llvm::const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    const llvm::Instruction& Inst = *I;
    if (llvm::isa<llvm::CallInst>(Inst) || llvm::isa<llvm::InvokeInst>(Inst)) {
			CallStack.push_back(&Inst);
      llvm::ImmutableCallSite cs(&Inst);
      // check if function call can be resolved statically
      if (cs.getCalledFunction() != nullptr) {
      	if (function_vertex_map.find(cs.getCalledFunction()->getName().str()) == function_vertex_map.end()) {
      		function_vertex_map[cs.getCalledFunction()->getName().str()] = boost::add_vertex(cg);
      		cg[function_vertex_map[cs.getCalledFunction()->getName().str()]] = VertexProperties(cs.getCalledFunction(),
																																															cs.getCalledFunction()->isDeclaration());
      	}
      	boost::add_edge(function_vertex_map[F->getName().str()], function_vertex_map[cs.getCalledFunction()->getName().str()], EdgeProperties(cs.getInstruction()), cg);
    		resolveIndirectCallWalkerSimple(cs.getCalledFunction());
	    } else { // the function call must be resolved dynamically
        set<string> possible_target_names = resolveIndirectCallOTF(cs);
        set<const llvm::Function*> possible_targets;
        for (auto& possible_target_name : possible_target_names) {
        	possible_targets.insert(IRDB.getFunction(possible_target_name));
        }
    		for (auto possible_target : possible_targets) {
    			if (function_vertex_map.find(possible_target->getName().str()) == function_vertex_map.end()) {
    				function_vertex_map[possible_target->getName().str()] = boost::add_vertex(cg);
    				cg[function_vertex_map[possible_target->getName().str()]] = VertexProperties(possible_target);
    			}
    			boost::add_edge(function_vertex_map[F->getName().str()], function_vertex_map[possible_target->getName().str()], EdgeProperties(cs.getInstruction()), cg);
    		}
    		// continue resolving
        for (auto possible_target : possible_targets) {
        	resolveIndirectCallWalkerSimple(possible_target);
        }
      }
    }
  }
	CallStack.pop_back();
}

void LLVMBasedICFG::resolveIndirectCallWalkerTypeAnalysis(
    const llvm::Function *F,
    function<set<string>(llvm::ImmutableCallSite CS)> R,
    bool useVTA) {

}

void LLVMBasedICFG::resolveIndirectCallWalkerPointerAnalysis(const llvm::Function* F) {
	auto& lg = lg::get();
	// do not analyze functions more than once (this also acts as recursion detection)
	if (VisitedFunctions.find(F) != VisitedFunctions.end() || F->isDeclaration()) {
		return;
	}
	VisitedFunctions.insert(F);
	// add a node for function F to the call graph (if not present already)
	if (function_vertex_map.find(F->getName().str()) == function_vertex_map.end()) {
  	function_vertex_map[F->getName().str()] = boost::add_vertex(cg);
  	cg[function_vertex_map[F->getName().str()]] = VertexProperties(F);
  }
  for (llvm::const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    const llvm::Instruction& Inst = *I;
    if (llvm::isa<llvm::CallInst>(Inst) || llvm::isa<llvm::InvokeInst>(Inst)) {
      llvm::ImmutableCallSite cs(&Inst);
      // function can be resolved statically
      if (cs.getCalledFunction() != nullptr) {
      	// get the points-to graph of the function that is called
      	PointsToGraph& callee_ptg = *IRDB.getPointsToGraph(cs.getCalledFunction()->getName().str());
      	callee_ptg.printAsDot(cs.getCalledFunction()->getName().str()+".dot");
      	// map the actual into formal parameters from caller to callee
      	auto escaping_formal_params = callee_ptg.getPointersEscapingThroughParams();
      	vector<pair<const llvm::Value*, const llvm::Value*>> mapping;
      	for (auto& entry : escaping_formal_params) {
      		mapping.push_back(make_pair(cs.getArgOperand(entry.first), entry.second));
      	}
      	// map the possibly multiple return values
      	auto escaping_return_vlaues = callee_ptg.getPointersEscapingThroughReturns();
      	for (auto user : cs->users()) {
      		for (auto escaping_return_value : escaping_return_vlaues) {
      			mapping.push_back(make_pair(user, escaping_return_value));
      		}
      	}
      	// note that aliasing with global variables is handled in the intra-procedural ptg construction
      	cout << "mapping caller to callee pointers\n";
      	printPTGMapping(mapping);
      	// additionally add a node for the target method to the graph (if not present already)
      	if (function_vertex_map.find(cs.getCalledFunction()->getName().str()) == function_vertex_map.end()) {
      		function_vertex_map[cs.getCalledFunction()->getName().str()] = boost::add_vertex(cg);
      		cg[function_vertex_map[cs.getCalledFunction()->getName().str()]] = VertexProperties(cs.getCalledFunction());
      	}
      	boost::add_edge(function_vertex_map[F->getName().str()], function_vertex_map[cs.getCalledFunction()->getName().str()], EdgeProperties(cs.getInstruction()), cg);
    		// do the merge
      	WholeModulePTG.mergeWith(callee_ptg, mapping, cs.getInstruction());
	      resolveIndirectCallWalkerPointerAnalysis(cs.getCalledFunction());
	    } else {
      // we have to resolve the called function ourselves using the accessible points-to information
				BOOST_LOG_SEV(lg, DEBUG) << "Found indirect call-site: " << llvmIRToString(cs.getInstruction());
        set<string> possible_target_names = resolveIndirectCallOTF(cs);
        set<const llvm::Function*> possible_targets;
        for (auto& possible_target_name : possible_target_names) {
        	possible_targets.insert(IRDB.getFunction(possible_target_name));
        }
        cout << "POSSIBLE TARGETS: " << possible_targets.size() << endl;
        // additionally add into boost graph
    		for (auto possible_target : possible_targets) {
    			if (function_vertex_map.find(possible_target->getName().str()) == function_vertex_map.end()) {
    				function_vertex_map[possible_target->getName().str()] = boost::add_vertex(cg);
    				cg[function_vertex_map[possible_target->getName().str()]] = VertexProperties(possible_target);
    			}
    			boost::add_edge(function_vertex_map[F->getName().str()], function_vertex_map[possible_target->getName().str()], EdgeProperties(cs.getInstruction()), cg);
    		}
    		// continue resolving
        for (auto possible_target : possible_targets) {
        	resolveIndirectCallWalkerPointerAnalysis(possible_target);
        }
      }
    }
  }
}

set<string> LLVMBasedICFG::resolveIndirectCallOTF(llvm::ImmutableCallSite CS) {
	auto& lg = lg::get();
	BOOST_LOG_SEV(lg, DEBUG) << "Resolve call";
  set<string> possible_call_targets;
  // check if we have a virtual call-site
  if ( CS.getNumArgOperands() > 0) {
    // deal with a virtual member function
  	// retrieve the vtable entry that is called
    const llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(CS.getCalledValue());
    const llvm::GetElementPtrInst* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(load->getPointerOperand());
    unsigned vtable_index = llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(1))->getZExtValue();
		BOOST_LOG_SEV(lg, DEBUG) << "Virtual function table entry is: " << vtable_index;
    const llvm::Value* receiver = CS.getArgOperand(0);
    const llvm::Type* receiver_type = receiver->getType();
    auto alloc_sites = WholeModulePTG.getReachableAllocationSites(receiver, CallStack);
    auto possible_types = WholeModulePTG.computeTypesFromAllocationSites(alloc_sites);
    if (possible_types.empty()) {
			BOOST_LOG_SEV(lg, WARNING) << "Could not find any possible type, using fall-back solution";
    	// here we take the conservative fall-back solution
    	if (const llvm::StructType* struct_type = llvm::dyn_cast<llvm::StructType>(receiver_type)) {
    		// insert declared type
    		// here we have to call debasify() since the following IR might be generated:
    		//   %struct.A = type <{ i32 (...)**, i32, [4 x i8] }>
    		//   %struct.B = type { %struct.A.base, [4 x i8] }
    		//   %struct.A.base = type <{ i32 (...)**, i32 }>
    		// in such a case %struct.A and %struct.A.base are treated as aliases and the vtable for A is stored
    		// under the name %struct.A whereas in the class hierarchy %struct.A.base is used!
    		// debasify() fixes that circumstance and returns it for all normal cases.
    		possible_call_targets.insert(CH.getVTableEntry(debasify(struct_type->getName().str()), vtable_index));
    		// also insert all possible subtypes
    		auto fallback_type_names = CH.getTransitivelyReachableTypes(struct_type->getName().str());
    		for (auto& fallback_name : fallback_type_names) {
    			possible_call_targets.insert(CH.getVTableEntry(fallback_name, vtable_index));
    		}
    	}
    } else {
    	cout << "found the following possible types" << endl;
    	for (auto type : possible_types) {
    		type->dump();
    		// caution: type might be a pointer type returned by a allocating function
    		const llvm::StructType* struct_type = (!type->isPointerTy()) ?
    																					llvm::dyn_cast<llvm::StructType>(type) :
																							llvm::dyn_cast<llvm::StructType>(type->getPointerElementType());
    		if (struct_type) {
    			// same as above
    			possible_call_targets.insert(CH.getVTableEntry(debasify(struct_type->getName().str()), vtable_index));
    		}
    	}
    }
    cout << "possible targets are:" << endl;
    for (auto entry : possible_call_targets) {
    	cout << entry << endl;
    }


  } else { // else we have to deal with a function pointer
   cout << "function pointer" << endl;
   if (CS.getCalledValue()->getType()->isPointerTy()) {
  	if (const llvm::FunctionType* ftype = llvm::dyn_cast<llvm::FunctionType>(CS.getCalledValue()->getType())) {
  		for (auto entry : IRDB.functions) {
  			const llvm::Function* f = IRDB.modules[entry.second]->getFunction(entry.first);
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
	return {};
}

set<string> LLVMBasedICFG::resolveIndirectCallRTA(llvm::ImmutableCallSite CS) {
	return {};
}

set<string> LLVMBasedICFG::resolveIndirectCallTA(llvm::ImmutableCallSite CS) {
	return {};
}


void LLVMBasedICFG::printPTGMapping(vector<pair<const llvm::Value*, const llvm::Value*>> mapping) {
	for (auto& entry : mapping) {
		cout << llvmIRToString(entry.first) << " ---> " << llvmIRToString(entry.second) << "\n";
	}
}

bool LLVMBasedICFG::isMemberFunctionCall(const llvm::Instruction *I) {
	set<const llvm::Function *> Callees = getCalleesOfCallAt(I);
	for (auto F : Callees) {
		if (F->getArgumentList().size()) {
    	auto Type = F->getArgumentList().begin()->getType();
    	if (Type->isPointerTy()) {
      	Type = Type->getPointerElementType();
      	if (auto ST = llvm::dyn_cast<llvm::StructType>(Type)) {
        	return CH.containsType(ST->getName().str());
      	}
    	}
  	}
	}
	return false;
}

const llvm::Function* LLVMBasedICFG::getMethodOf(const llvm::Instruction* stmt) {
	return stmt->getFunction();
}

const llvm::Function* LLVMBasedICFG::getMethod(const string& fun) {
	return IRDB.getFunction(fun);
}

vector<const llvm::Instruction*> LLVMBasedICFG::getPredsOf(const llvm::Instruction* I) {
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

vector<const llvm::Instruction*> LLVMBasedICFG::getSuccsOf(const llvm::Instruction* I) {
	vector<const llvm::Instruction*> Successors;
	if (I->getNextNode())
	  Successors.push_back(I->getNextNode());
	if (const llvm::TerminatorInst* T = llvm::dyn_cast<llvm::TerminatorInst>(I)) {
	  for (auto successor : T->successors()) {
	   Successors.push_back(&*successor->begin());
	  }
	}
	return Successors;
}

vector<pair<const llvm::Instruction*,const llvm::Instruction*>> LLVMBasedICFG::getAllControlFlowEdges(const llvm::Function* fun) {
	vector<pair<const llvm::Instruction*,const llvm::Instruction*>> Edges;
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

vector<const llvm::Instruction*> LLVMBasedICFG::getAllInstructionsOf(const llvm::Function* fun) {
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

bool LLVMBasedICFG::isFallThroughSuccessor(const llvm::Instruction* stmt, const llvm::Instruction* succ) {
	if (const llvm::BranchInst* B = llvm::dyn_cast<llvm::BranchInst>(succ)) {
    if (B->isConditional()) {
       return &B->getSuccessor(1)->front() == succ;
     } else {
       return &B->getSuccessor(0)->front() == succ;
     }
	}
	return false;
}

bool LLVMBasedICFG::isBranchTarget(const llvm::Instruction* stmt, const llvm::Instruction* succ) {
	if (const llvm::TerminatorInst* T = llvm::dyn_cast<llvm::TerminatorInst>(stmt)) {
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
  		return { CS.getCalledFunction() };
  	} else { // handle indirect call
			set<const llvm::Function*> Callees;
			string CallerName = CS->getFunction()->getName().str();
			out_edge_iterator ei, ei_end;
  		for (boost::tie(ei, ei_end) = boost::out_edges(function_vertex_map[CallerName], cg); ei != ei_end; ++ei) {
    		auto source = boost::source( *ei, cg );
				auto edge = cg[*ei];
    		auto target = boost::target( *ei, cg );
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
	for (boost::tie(ei, ei_end) = boost::in_edges(function_vertex_map[m->getName().str()], cg); ei != ei_end; ++ei) {
  	auto source = boost::source( *ei, cg );
		auto edge = cg[*ei];
    auto target = boost::target( *ei, cg );
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
  for (llvm::const_inst_iterator I = llvm::inst_begin(f), E = llvm::inst_end(f); I != E; ++I) {
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
  return { &m->front().front() };
}

set<const llvm::Instruction*> LLVMBasedICFG::getExitPointsOf(
		const llvm::Function* fun) {
	return { &fun->back().back() };
}

/**
 * Returns all statements to which a call could return.
 * In the RHS paper, for every call there is just one return site.
 * We, however, use as return site the successor statements, of which
 * there can be many in case of exceptional flow.
 */
set<const llvm::Instruction*>
LLVMBasedICFG::getReturnSitesOfCallAt(
    const llvm::Instruction* n) {
  // at the moment we just ignore exceptional control flow
  set<const llvm::Instruction*> ReturnSites;
  if (llvm::isa<llvm::CallInst>(n) || llvm::isa<llvm::InvokeInst>(n)) {
    //ReturnSites.insert(n);
  	llvm::ImmutableCallSite CS(n);
  	for (auto user : CS->users()) {
  		if (const llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(user)) {
  			ReturnSites.insert(inst);
  		}
  	}
  	if (ReturnSites.empty()) {
  		ReturnSites.insert(n->getNextNode());
  	}
  }
  return ReturnSites;
}

CallType LLVMBasedICFG::isCallStmt(const llvm::Instruction* stmt) {
	if (llvm::isa<llvm::CallInst>(stmt) || llvm::isa<llvm::InvokeInst>(stmt)) {
		set<const llvm::Function*> Callees = getCalleesOfCallAt(stmt);
		for (auto Callee : Callees) {
			if (Callee->isDeclaration()) {
				return CallType::unavailable;
			}
		}
		return CallType::call;
	} else {
		return CallType::none;
	}
}

/**
 * Returns the set of all nodes that are neither call nor start nodes.
 */
set<const llvm::Instruction*>
LLVMBasedICFG::allNonCallStartNodes() {
  set<const llvm::Instruction*> NonCallStartNodes;
	for (auto M : IRDB.getAllModules()) {
  	for (auto& F : *M) {
  		for (auto& BB : F) {
  			for (auto& I : BB) {
  				if ((!llvm::isa<llvm::CallInst>(&I)) && (!llvm::isa<llvm::InvokeInst>(&I)) && (!isStartPoint(&I))) {
  					NonCallStartNodes.insert(&I);
  				}
  			}
  		}
  	}
	}
  return NonCallStartNodes;
}

vector<const llvm::Instruction*>
LLVMBasedICFG::getAllInstructionsOfFunction(const string& name) {
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
	// This vector holds the vertices to be merged in (decl,defn) or (defn, decl) pairs
	vector<pair<LLVMBasedICFG::vertex_t, LLVMBasedICFG::vertex_t>> v_in_g1_u_in_g2;
	vertex_iterator vi, vi_end;
	for (boost::tie(vi, vi_end) = boost::vertices(cg); vi != vi_end; ++vi) {
		cout << cg[*vi].functionName << '\n';
	}
	// Also we must merge the points-to graphs
	// TODO
 	// Merge the already visited functions
	VisitedFunctions.insert(other.VisitedFunctions.begin(), other.VisitedFunctions.end());
  // Perform the actual call-graph merge
	// TODO
	//	void merge_graphs(GraphTy& g1, const GraphTy& g2, vector<pair<VertexTy, VertexTy>> v_in_g1_u_in_g2, Args&&... args)
	// merge_graphs(cg, other.cg, v_in_g1_u_in_g2, /* call-site */ nullptr);
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
	boost::print_graph(cg, boost::get(&LLVMBasedICFG::VertexProperties::functionName, cg));
}

void LLVMBasedICFG::printAsDot(const string& filename) {
	ofstream ofs(filename);
	boost::write_graphviz(ofs, cg,
			boost::make_label_writer(boost::get(&LLVMBasedICFG::VertexProperties::functionName, cg)),
			boost::make_label_writer(boost::get(&LLVMBasedICFG::EdgeProperties::ir_code, cg)));
}

void LLVMBasedICFG::exportPATBCJSON() {
	cout << "LLVMBasedICFG::exportPATBCJSON()\n";
}
