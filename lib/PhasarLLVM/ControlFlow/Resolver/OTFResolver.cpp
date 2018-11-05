/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 *  DTAResolver.cpp
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/OTFResolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;

OTFResolver::OTFResolver(ProjectIRDB &irdb, LLVMTypeHierarchy &ch,
                         PointsToGraph &wholemodulePTG)
    : CHAResolver(irdb, ch), WholeModulePTG(wholemodulePTG) {}

void OTFResolver::preCall(const llvm::Instruction *Inst) {
  CallStack.push_back(Inst);
}

void OTFResolver::TreatPossibleTarget(
    const llvm::ImmutableCallSite &CS,
    std::set<const llvm::Function *> &possible_targets) {
  auto &lg = lg::get();

  for (auto possible_target : possible_targets) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Target name: " << possible_target->getName().str());
    // Do the merge of the points-to graphs for all possible targets, but
    // only if they are available

    if (auto F = CS.getCaller()) {
      if (auto M = F->getParent()) {
        if (auto target = M->getFunction(possible_target->getName().str())) {
          if (!target->isDeclaration()) {
            PointsToGraph &callee_ptg =
                *IRDB.getPointsToGraph(possible_target->getName().str());
            WholeModulePTG.mergeWith(callee_ptg, CS, possible_target);
          }
        } else {
          throw runtime_error("target not get");
        }
      } else {
        throw runtime_error("M not get");
      }
    } else {
      throw runtime_error("F not get");
    }
  }
}

void OTFResolver::postCall(const llvm::Instruction *Inst) {
  CallStack.pop_back();
}

void OTFResolver::OtherInst(const llvm::Instruction *Inst) {}

set<string> OTFResolver::resolveVirtualCall(const llvm::ImmutableCallSite &CS) {
  set<string> possible_call_targets;
  auto &lg = lg::get();

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Call virtual function: "
                << llvmIRToString(CS.getInstruction()));

  auto vtable_index = this->getVtableIndex(CS);
  if (vtable_index < 0) {
    // An error occured
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                  << llvmIRToString(CS.getInstruction()) << "\n");
    return {};
  }

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Virtual function table entry is: " << vtable_index);

  const llvm::Value *receiver = CS.getArgOperand(0);

  auto alloc_sites =
      WholeModulePTG.getReachableAllocationSites(receiver, CallStack);
  auto possible_allocated_types =
      WholeModulePTG.computeTypesFromAllocationSites(alloc_sites);

  auto receiver_type = this->getReceiverType(CS);

  // Now we must check if we have found some allocated struct types
  set<const llvm::StructType *> possible_types;
  for (auto type : possible_allocated_types) {
    if (auto struct_type =
            llvm::dyn_cast<llvm::StructType>(stripPointer(type))) {
      possible_types.insert(struct_type);
    }
  }

  for (auto possible_type_struct : possible_types) {
    string type_name = possible_type_struct->getName().str();
    insertVtableIntoResult(possible_call_targets, type_name, vtable_index, CS);
  }

  if (possible_call_targets.empty())
    return CHAResolver::resolveVirtualCall(CS);

  return possible_call_targets;
}
