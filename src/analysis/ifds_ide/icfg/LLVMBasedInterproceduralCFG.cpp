/*
 * LLVMBasedInterproceduralICFG.cpp
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#include "LLVMBasedInterproceduralCFG.hh"

void LLVMBasedInterproceduralICFG::resolveIndirectCallWalker(
    const llvm::Function* F) {
  PointsToGraph ptg(AA, F);
  cout << "walking in " << F->getName().str() << endl;

  for (llvm::const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E;
       ++I) {
    const llvm::Instruction& Inst = *I;
    if (llvm::isa<llvm::CallInst>(Inst) || llvm::isa<llvm::InvokeInst>(Inst)) {
      llvm::ImmutableCallSite cs(llvm::dyn_cast<llvm::CallInst>(&Inst));
      cout << "call site" << endl;
      // function can be resolved statically
      if (cs.getCalledFunction() != nullptr) {
        resolveIndirectCallWalker(cs.getCalledFunction());
      } else {
        // we have to resolve the called function ourselves
        set<const llvm::Function*> possible_targets = resolveIndirectCall(cs);
        for (auto target : possible_targets) {
          PointsToGraph ptg_called_fun(AA, target);
          resolveIndirectCallWalker(target);
        }
      }
    } else if (llvm::isa<llvm::ReturnInst>(Inst)) {
      cout << "return" << endl;
    } else {
      cout << "other" << endl;
    }
  }
}

set<const llvm::Function*> LLVMBasedInterproceduralICFG::resolveIndirectCall(
    llvm::ImmutableCallSite CS) {
  cout << "RESOLVING CALL" << endl;
  set<const llvm::Type*> possible_receiver_types;
  set<const llvm::Function*> possible_call_targets;
//   LLVMStructTypeHierarchy CH;
//   CH.analyzeModule(M);
//   if (CS.getNumArgOperands() > 0) {
//     // deal with a virtual member function
//     const llvm::LoadInst* load =
//         llvm::dyn_cast<llvm::LoadInst>(CS.getCalledValue());
//     const llvm::GetElementPtrInst* gep =
//         llvm::dyn_cast<llvm::GetElementPtrInst>(load->getPointerOperand());
//     unsigned vtable_entry =
//         llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(1))->getZExtValue();

//     const llvm::Value* receiver = CS.getArgOperand(0);
//     const llvm::Type* receiver_type = receiver->getType();
//     cout << "receiver" << endl;
//     receiver->dump();
//     cout << "receiver type" << endl;
//     receiver_type->dump();
//     // retrieve the plain type
//     // only one dereferencing should be necessary otherwise something is broken
//     // elsewhere
//     receiver_type = receiver_type->getPointerElementType();
//     const llvm::Function* enclosing_function = CS.getParent()->getParent();
//     cout << "begin class hierarchy" << endl;
//   //  cout << CH << endl;
//     cout << "end class hierarchy" << endl;
//     auto points_to_set = P.getPointsToSet(enclosing_function, receiver);
//     cout << "points to set" << endl;
//     for (auto pointer : points_to_set) {
//       // cout << "obtained from bitcast: " <<
//       // llvm::isa<llvm::BitCastInst>(pointer) << endl;
//       // here we want to ignore values casted to the receiver super type, since
//       // we know that anyway
//       if (const llvm::BitCastInst* cast =
//               llvm::dyn_cast<llvm::BitCastInst>(pointer)) {
//         const llvm::Type* destty = cast->getDestTy();
//         if (destty->isPointerTy()) {
//           destty = destty->getPointerElementType();
//         }
//         if (destty == receiver_type) {
//           continue;
//         }
//       }
//       pointer->dump();
//       //			pointer->getType()->dump();
//       //			auto formal_aliases =
//       //P.aliasWithFormalParameter(CS.getParent()->getParent(), pointer);
//       //			if (!formal_aliases.empty()) {
//       //				//cout << "  aliases with formal" <<
//       //endl;
//       //			}
//       // retrieve the plain type
//       const llvm::Type* plain_type = pointer->getType();
//       if (plain_type->isPointerTy()) {
//         // only dereference once, since receiver object is always a pointer to
//         // type T (not pointer to pointer to ...)
//         // M.foo() ---> foo(M* m);
//         plain_type = plain_type->getPointerElementType();
//       }
//       plain_type->dump();
//       // if (CH.hasSubType(receiver_type, plain_type)) {
//       //   possible_receiver_types.insert(plain_type);
//       // }
//     }
//     // check if we could not find some possible receiver types
//     // in this case we could not retrieve any useful information and thus must
//     // assume that any version
//     // of one of receivers children could be called
//     if (possible_receiver_types.empty()) {
//       set<const llvm::Type*> fallback;// =
//         // change this
//         //  CH.getTransitivelyReachableTypes(receiver_type);
//       possible_receiver_types.insert(fallback.begin(), fallback.end());
//       possible_receiver_types.insert(receiver_type);
//     }
//     for (auto possible_receiver_type : possible_receiver_types) {
//       // possible_receiver_type->dump();
//  //     auto vtable = CH.constructVTable(possible_receiver_type, &M);
//       // use at() instead of operator[] to be sure, that an exception is thrown
//       // if this works not as expected
//       if (!vtable.empty()) {
//         possible_call_targets.insert(vtable.at(vtable_entry));
//       }
//     }
//   } else {
//     // deal with a function pointer
//     //		set<const llvm::Value*> points_to_set =
//     //P.getPointsToSet(CS->getParent()->getParent(), CS.getCalledValue());
//     //		for (auto pointer : points_to_set) {
//     //			if(const llvm::LoadInst* load =
//     //llvm::dyn_cast<llvm::LoadInst>(pointer)) {
//     //				load->getPointerOperand()
//     //			}
//     //		}
//   }
  return possible_call_targets;
}

const llvm::Function* LLVMBasedInterproceduralICFG::getMethodOf(
    const llvm::Instruction* n) {
  return n->getFunction();
}

vector<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getPredsOf(
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

vector<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getSuccsOf(
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
set<const llvm::Function*> LLVMBasedInterproceduralICFG::getCalleesOfCallAt(
    const llvm::Instruction* n) {
  if (const llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(n)) {
    // handle call
    if (CI->getCalledFunction()) {
      // called function can be statically resolved
      return {CI->getCalledFunction()};
    } else {
      // called function needs to be resolved
      return resolveIndirectCall(llvm::ImmutableCallSite{CI});
    }
  } else if (const llvm::InvokeInst* II = llvm::dyn_cast<llvm::InvokeInst>(n)) {
    // handle invoke
    if (II->getCalledFunction()) {
      // invoked function can be statically resolved
      return {II->getCalledFunction()};
    } else {
      // invoked function needs to be resolved
      return resolveIndirectCall(llvm::ImmutableCallSite{II});
    }
  } else {
    // neither call nor invoke - error!
    llvm::errs()
        << "ERROR: found instruction that is neither CallInst nor InvokeInst\n";
    return {};
  }
}

/**
 * Returns all caller statements/nodes of a given method.
 */
set<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getCallersOf(
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
  return CallersOf;
}

/**
 * Returns all call sites within a given method.
 */
set<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getCallsFromWithin(
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
set<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getStartPointsOf(
    const llvm::Function* m) {
  set<const llvm::Instruction*> StartPoints;
  // this does not handle backwards analysis, where a function may contains more
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
LLVMBasedInterproceduralICFG::getReturnSitesOfCallAt(
    const llvm::Instruction* n) {
  // at the moment we just ignore exceptional control flow
  set<const llvm::Instruction*> RetSites;
  if (llvm::isa<llvm::CallInst>(n)) {
    RetSites.insert(n->getNextNode());
  }
  return RetSites;
}

bool LLVMBasedInterproceduralICFG::isCallStmt(const llvm::Instruction* stmt) {
  return llvm::isa<llvm::CallInst>(stmt);
}

bool LLVMBasedInterproceduralICFG::isExitStmt(const llvm::Instruction* stmt) {
  return llvm::isa<llvm::ReturnInst>(stmt);
}

/**
 * Returns all start points of a given method. There may be
 * more than one start point in case of a backward analysis.
 */
bool LLVMBasedInterproceduralICFG::isStartPoint(const llvm::Instruction* stmt) {
  const llvm::Function* F = stmt->getFunction();
  return F->getEntryBlock().getFirstNonPHIOrDbg() == stmt;
}

/**
 * Returns the set of all nodes that are neither call nor start nodes.
 */
set<const llvm::Instruction*>
LLVMBasedInterproceduralICFG::allNonCallStartNodes() {
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
bool LLVMBasedInterproceduralICFG::isFallThroughSuccessor(
    const llvm::Instruction* stmt, const llvm::Instruction* succ) {
  return (stmt->getParent() == succ->getParent());
}

/**
 * Returns whether succ is a branch target of stmt.
 */
bool LLVMBasedInterproceduralICFG::isBranchTarget(
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
LLVMBasedInterproceduralICFG::getAllInstructionsOfFunction(const string& name) {
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

const llvm::Instruction* LLVMBasedInterproceduralICFG::getLastInstructionOf(
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
LLVMBasedInterproceduralICFG::getAllInstructionsOfFunction(
    const llvm::Function* func) {
  return getAllInstructionsOfFunction(func->getName().str());
}

const string LLVMBasedInterproceduralICFG::getNameOfMethod(
    const llvm::Instruction* stmt) {
  return stmt->getFunction()->getName().str();
}
