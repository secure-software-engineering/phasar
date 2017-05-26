/*
 * LLVMBasedInterproceduralICFG.cpp
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#include "LLVMBasedICFG.hh"

LLVMBasedICFG::LLVMBasedICFG(
    llvm::Module& Module, LLVMStructTypeHierarchy& STH,
    ProjectIRCompiledDB& IRDB)
    : M(Module), CG(Module), CH(STH), IRDB(IRDB) {
  // perform the resolving of all dynamic call sites contained in the corresponding module
	llvm::Function* main = M.getFunction("main");
	cout << "calling the walker ...\n";
	if (main) {
		PointsToGraph& main_ptg = *IRDB.getPointsToGraph("main");
		WholeModulePTG.mergeWith(main_ptg, {}, nullptr);
		WholeModulePTG.printAsDot("main_ptg.dot");
		resolveIndirectCallWalker(main);
	} else {
		cout << "could not find 'main()' function in call graph construction!\n";
	}
	cout << "constructed whole module ptg and resolved indirect calls ...\n";
	WholeModulePTG.printAsDot("whole_module_ptg.dot");
}

LLVMBasedICFG::LLVMBasedICFG(llvm::Module& Module,
							LLVMStructTypeHierarchy& STH,
							ProjectIRCompiledDB& IRDB,
							const vector<string>& EntryPoints)
		: M(Module), CG(Module), CH(STH), IRDB(IRDB) {
	for (auto& function_name : EntryPoints) {
			llvm::Function* function = M.getFunction(function_name);
			PointsToGraph& ptg = *IRDB.getPointsToGraph(function_name);
			WholeModulePTG.mergeWith(ptg, {}, nullptr);
			resolveIndirectCallWalker(function);
	}
}

void LLVMBasedICFG::resolveIndirectCallWalker(const llvm::Function* F) {
  for (llvm::const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    const llvm::Instruction& Inst = *I;
    if (llvm::isa<llvm::CallInst>(Inst) || llvm::isa<llvm::InvokeInst>(Inst)) {
      llvm::ImmutableCallSite cs(llvm::dyn_cast<llvm::CallInst>(&Inst));
      // function can be resolved statically
      if (cs.getCalledFunction() != nullptr) {
      	// get the ptg of the function that is called
      	PointsToGraph& callee_ptg = *IRDB.getPointsToGraph(cs.getCalledFunction()->getName().str());
      	callee_ptg.printAsDot(cs.getCalledFunction()->getName().str()+".dot");
      	// map the formals
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
      	DirectCSTargetMethods.insert(make_pair(&Inst, cs.getCalledFunction()));
      	// do the merge
      	WholeModulePTG.mergeWith(callee_ptg, mapping, cs.getInstruction());
      	resolveIndirectCallWalker(cs.getCalledFunction());
      } else {
      // we have to resolve the called function ourselves using the accessible points-to information
        cout << "FOUND INDIRECT CALL-SITE" << endl;
        cs->dump();
        set<string> possible_target_names = resolveIndirectCall(cs);
        set<const llvm::Function*> possible_targets;
        for (auto& possible_target_name : possible_target_names) {
        	possible_targets.insert(IRDB.getFunction(possible_target_name));
        }
        IndirectCSTargetMethods.insert(make_pair(cs.getInstruction(), possible_targets));
        for (auto possible_target : possible_targets) {
        	resolveIndirectCallWalker(possible_target);
        }
      }
    }
  }
}

set<string> LLVMBasedICFG::resolveIndirectCall(llvm::ImmutableCallSite CS) {
  cout << "RESOLVING CALL" << endl;
  set<string> possible_call_targets;
  // check if we have a virtual call-site
  if ( CS.getNumArgOperands() > 0) {
    // deal with a virtual member function
  	// retrieve the vtable entry that is called
    const llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(CS.getCalledValue());
    const llvm::GetElementPtrInst* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(load->getPointerOperand());
    unsigned vtable_index = llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(1))->getZExtValue();

    cout << "vtable entry: " << vtable_index << endl;

    const llvm::Value* receiver = CS.getArgOperand(0);
    const llvm::Type* receiver_type = receiver->getType();
    auto alloc_sites = WholeModulePTG.getReachableAllocationSites(receiver);
    auto possible_types = WholeModulePTG.computeTypesFromAllocationSites(alloc_sites);
    if (possible_types.empty()) {
    	cout << "could not find any possible types" << endl;
    	// here we take the conservative fall-back solution
    	if (const llvm::StructType* struct_type = llvm::dyn_cast<llvm::StructType>(receiver_type)) {
    		// insert declared type
    		// here we have to call debasify() since the following IR might be generated:
    		//   %struct.A = type <{ i32 (...)**, i32, [4 x i8] }>
    		//   %struct.B = type { %struct.A.base, [4 x i8] }
    		//   %struct.A.base = type <{ i32 (...)**, i32 }>
    		// in such a case %struct.A and %struct.A.base are treated as aliases and the vtable for A is stored
    		// under the name %struct.A whereas in the class hierarchy %struct.A.base is used!
    		// debasify() fixes that circumstance and returns id for all normal cases.
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

void LLVMBasedICFG::printPTGMapping(vector<pair<const llvm::Value*, const llvm::Value*>> mapping) {
	for (auto& entry : mapping) {
		cout << llvmIRToString(entry.first) << " ---> " << llvmIRToString(entry.second) << "\n";
	}
}

const llvm::Function* LLVMBasedICFG::getMethodOf(
    const llvm::Instruction* n) {
  return n->getFunction();
}

vector<const llvm::Instruction*> LLVMBasedICFG::getPredsOf(
    const llvm::Instruction* u) {
  // FIXME
  cout << "getPredsOf not supported yet" << endl;
  vector<const llvm::Instruction*> IVec;
  const llvm::BasicBlock* BB = u->getParent();
  if (BB->getFirstNonPHIOrDbg() != u) {
  } else {
    IVec.push_back(u->getPrevNode());
  }
  return IVec;
}

vector<const llvm::Instruction*> LLVMBasedICFG::getSuccsOf(
    const llvm::Instruction* n) {
  vector<const llvm::Instruction*> IVec;
  // if n is a return instruction, there are no more successors
  if (llvm::isa<llvm::ReturnInst>(n)) return IVec;
  // get the next instruction
  const llvm::Instruction* I = n->getNextNode();
  // check if the next instruction is not a branch instruction just add it to
  // the successors
  if (!llvm::isa<llvm::BranchInst>(I)) {
    IVec.push_back(I);
  } else {
    // if it is a branch instruction there are multiple successors we have to
    // collect
    const llvm::BranchInst* BI = llvm::dyn_cast<const llvm::BranchInst>(I);
    for (size_t i = 0; i < BI->getNumSuccessors(); ++i)
      IVec.push_back(BI->getSuccessor(i)->getFirstNonPHIOrDbg());
  }
  return IVec;
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
  		return IndirectCSTargetMethods[n];
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
  for (auto& entry : DirectCSTargetMethods) {
  	if (entry.second == m) {
  		CallersOf.insert(entry.first);
  	}
  }
  for (auto& entry : IndirectCSTargetMethods) {
  	for (auto function : entry.second) {
  		if (function == m) {
  			CallersOf.insert(entry.first);
  		}
  	}
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
  set<const llvm::Instruction*> StartPoints;
  // this does not handle backwards analysis, where a function may contains
  // more
  // than one start points!
  StartPoints.insert(m->getEntryBlock().getFirstNonPHIOrDbg());
  return StartPoints;
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
			return CallType::call;
	} else {
		return CallType::none;
	}
}

bool LLVMBasedICFG::isExitStmt(const llvm::Instruction* stmt) {
  return llvm::isa<llvm::ReturnInst>(stmt);
}

/**
 * Returns all start points of a given method. There may be
 * more than one start point in case of a backward analysis.
 */
bool LLVMBasedICFG::isStartPoint(const llvm::Instruction* stmt) {
  const llvm::Function* F = stmt->getFunction();
  return F->getEntryBlock().getFirstNonPHIOrDbg() == stmt;
}

/**
 * Returns the set of all nodes that are neither call nor start nodes.
 */
set<const llvm::Instruction*>
LLVMBasedICFG::allNonCallStartNodes() {
  set<const llvm::Instruction*> NonCallStartNodes;
  for (llvm::Module::const_iterator MI = M.begin(); MI != M.end(); ++MI) {
    for (llvm::Function::const_iterator FI = MI->begin(); FI != MI->end();
         ++FI) {
      llvm::ilist_iterator<const llvm::BasicBlock> BB = FI;
      for (llvm::BasicBlock::const_iterator BI = BB->begin(), BE = BB->end();
           BI != BE;) {
        const llvm::Instruction& I = *BI++;
        if ((!llvm::isa<llvm::CallInst>(I)) && (!isStartPoint(&I)))
          NonCallStartNodes.insert(&I);
      }
    }
  }
  return NonCallStartNodes;
}

/**
 * Returns whether succ is the fall-through successor of stmt,
 * i.e., the unique successor that is be reached when stmt
 * does not branch.
 */
bool LLVMBasedICFG::isFallThroughSuccessor(
    const llvm::Instruction* stmt, const llvm::Instruction* succ) {
  return (stmt->getParent() == succ->getParent());
}

/**
 * Returns whether succ is a branch target of stmt.
 */
bool LLVMBasedICFG::isBranchTarget(
    const llvm::Instruction* stmt, const llvm::Instruction* succ) {
  if (const llvm::BranchInst* BI = llvm::dyn_cast<llvm::BranchInst>(stmt)) {
    for (size_t successor = 0; successor < BI->getNumSuccessors();
         ++successor) {
      const llvm::BasicBlock* BB = BI->getSuccessor(successor);
      if (BB == succ->getParent()) return true;
    }
  }
  return false;
}

vector<const llvm::Instruction*>
LLVMBasedICFG::getAllInstructionsOfFunction(const string& name) {
  vector<const llvm::Instruction*> IVec;
  const llvm::Function* f = M.getFunction(name);
  for (llvm::const_inst_iterator I = inst_begin(f), E = inst_end(f); I != E; ++I) {
  	IVec.push_back(&(*I));
  }
  return IVec;
}

const llvm::Instruction* LLVMBasedICFG::getLastInstructionOf(
    const string& name) {
  const llvm::Function& f = *M.getFunction(name);
  auto last = llvm::inst_end(f);
  last--;
  return &(*last);
}

vector<const llvm::Instruction*>
LLVMBasedICFG::getAllInstructionsOfFunction(
    const llvm::Function* func) {
  return getAllInstructionsOfFunction(func->getName().str());
}

string LLVMBasedICFG::getMethodName(const llvm::Function* F) {
  return F->getName().str();
}

string LLVMBasedICFG::getMethodName(const llvm::Instruction* n) {
	if (const llvm::CallInst* Call = llvm::dyn_cast<llvm::CallInst>(n)) {
			return Call->getCalledFunction()->getName().str();
	} else if (const llvm::InvokeInst* Invoke = llvm::dyn_cast<llvm::InvokeInst>(n)) {
			return Invoke->getCalledFunction()->getName().str();
	} else {
		return "";
	}
}

void LLVMBasedICFG::print() {
	cout << "LLVMBasedICFG\n";
	cout << "direct calls:\n";
	for (auto& entry : DirectCSTargetMethods) {
		entry.first->dump();
		cout << entry.second->getName().str() << "\n\n";
	}
	cout << "indirect calls:\n";
	for (auto& entry : IndirectCSTargetMethods) {
		entry.first->dump();
		cout << "possibilities:\n";
		for (auto& target : entry.second) {
			cout << target->getName().str() << "\n";
		}
	}
	cout << endl;
}
