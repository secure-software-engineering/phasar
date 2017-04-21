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
	// provide one or more functions as seed(s)
		set<llvm::Function*> seeds;
		seeds.insert(M.getFunction("main"));
		cout << "calling the walker ...\n";
		for (auto seed : seeds) {
			PointsToGraph& main_ptg = *IRDB.ptgs[M.getFunction("main")->getName().str()];
			WholeModulePTG.mergeWith(main_ptg, {}, nullptr);
			WholeModulePTG.printAsDot("main_ptg.dot");
			resolveIndirectCallWalker(seed);
		}

		cout << "constructed whole module ptg and resolved indirect calls ...\n";
		WholeModulePTG.printAsDot("whole_module_ptg.dot");
}

void LLVMBasedICFG::resolveIndirectCallWalker(const llvm::Function* F) {
  for (llvm::const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    const llvm::Instruction& Inst = *I;
    if (llvm::isa<llvm::CallInst>(Inst) || llvm::isa<llvm::InvokeInst>(Inst)) {
      llvm::ImmutableCallSite cs(llvm::dyn_cast<llvm::CallInst>(&Inst));
      // function can be resolved statically
      if (cs.getCalledFunction() != nullptr) {
      	// get the ptg of the function that is called
      	PointsToGraph& callee_ptg = *IRDB.ptgs[cs.getCalledFunction()->getName().str()];
      	callee_ptg.printAsDot("callee.dot");
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
//      	for (auto& entry : mapping) {
//      		cout << "beg map" << endl;
//      		entry.first->dump();
//      		entry.second->dump();
//      		cout << "end map" << endl;
//      	}
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
        	possible_targets.insert(IRDB.modules[IRDB.functions[possible_target_name]]->getFunction(possible_target_name));
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
    unsigned vtable_entry = llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(1))->getZExtValue();

    cout << "vtable entry: " << vtable_entry << endl;

    const llvm::Value* receiver = CS.getArgOperand(0);
    const llvm::Type* receiver_type = receiver->getType();
    auto alloc_sites = WholeModulePTG.getReachableAllocationSites(receiver);
    auto possible_types = WholeModulePTG.computeTypesFromAllocationSites(alloc_sites);
    if (possible_types.empty()) {
    	cout << "could not find any possible types" << endl;
    	// here we take the conservative fall-back solution
    	if (const llvm::StructType* struct_type = llvm::dyn_cast<llvm::StructType>(receiver_type)) {
    		// insert declared type
    		possible_call_targets.insert(CH.getVTableEntry(struct_type->getName().str(), vtable_entry));
    		// also insert all possible subtypes
    		auto fallback_type_names = CH.getTransitivelyReachableTypes(struct_type->getName().str());
    		for (auto& fallback_name : fallback_type_names) {
    			possible_call_targets.insert(CH.getVTableEntry(fallback_name, vtable_entry));
    		}
    	}
    } else {
    	cout << "found the following possible types" << endl;
    	for (auto type : possible_types) {
    		type->dump();
    		if (const llvm::StructType* struct_type = llvm::dyn_cast<llvm::StructType>(type)) {
    			possible_call_targets.insert(CH.getVTableEntry(struct_type->getName().str(), vtable_entry));
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
  for (llvm::Module::const_iterator MI = M.begin(); MI != M.end(); ++MI) {
    for (llvm::Function::const_iterator FI = MI->begin(); FI != MI->end();
         ++FI) {
      llvm::ilist_iterator<const llvm::BasicBlock> BB = FI;
      for (llvm::BasicBlock::const_iterator BBI = BB->begin(); BBI != BB->end();
           ++BBI) {
        const llvm::Instruction& I = *BBI;
        if (const llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
          if (CI->getCalledFunction() == m) {
            CallersOf.insert(&I);
          }
        }
      }
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
    const llvm::Function* m) {
  set<const llvm::Instruction*> ISet;
  for (llvm::Function::const_iterator FI = m->begin(); FI != m->end(); ++FI) {
    llvm::ilist_iterator<const llvm::BasicBlock> BB = FI;
    // iterate over single instructions
    for (llvm::BasicBlock::const_iterator BI = BB->begin(), BE = BB->end();
         BI != BE;) {
      const llvm::Instruction& I = *BI++;
      if (llvm::isa<llvm::CallInst>(I)) ISet.insert(&I);
    }
  }
  return ISet;
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
  set<const llvm::Instruction*> RetSites;
  if (llvm::isa<llvm::CallInst>(n)) {
    RetSites.insert(n->getNextNode());
  }
  return RetSites;
}

bool LLVMBasedICFG::isCallStmt(const llvm::Instruction* stmt) {
  return llvm::isa<llvm::CallInst>(stmt);
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
  const llvm::Function* func = M.getFunction(name);
  for (llvm::Function::const_iterator FI = func->begin(); FI != func->end();
       ++FI) {
    llvm::ilist_iterator<const llvm::BasicBlock> BB = FI;
    for (llvm::BasicBlock::const_iterator BI = BB->begin(), BE = BB->end();
         BI != BE;) {
      const llvm::Instruction& I = *BI++;
      IVec.push_back(&I);
    }
  }
  return IVec;
}

const llvm::Instruction* LLVMBasedICFG::getLastInstructionOf(
    const string& name) {
  const llvm::Function* func = M.getFunction(name);
  for (llvm::Function::const_iterator FI = func->begin(); FI != func->end();
       ++FI) {
    llvm::ilist_iterator<const llvm::BasicBlock> BB = FI;
    for (llvm::BasicBlock::const_iterator BI = BB->begin(), BE = BB->end();
         BI != BE;) {
      const llvm::Instruction& I = *BI++;
      if (llvm::isa<llvm::ReturnInst>(I)) return &I;
    }
  }
  return nullptr;
}

vector<const llvm::Instruction*>
LLVMBasedICFG::getAllInstructionsOfFunction(
    const llvm::Function* func) {
  return getAllInstructionsOfFunction(func->getName().str());
}

const string LLVMBasedICFG::getNameOfMethod(
    const llvm::Instruction* stmt) {
  return stmt->getFunction()->getName().str();
}
