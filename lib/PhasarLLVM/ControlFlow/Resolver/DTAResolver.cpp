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
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/DTAResolver.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Utilities.h>

using namespace std;
using namespace psr;

DTAResolver::DTAResolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH)
    : CHAResolver(IRDB, TH) {}

bool DTAResolver::heuristic_anti_contructor_this_type(
    const llvm::BitCastInst *bitcast) {
  // We check if the caller is a constructor, and if the this argument has the
  // same type as the source type of the bitcast. If it is the case, it returns
  // false, true otherwise.

  if (auto caller = bitcast->getFunction()) {
    if (isConstructor(caller->getName().str())) {
      if (auto func_ty = caller->getFunctionType()) {
        if (auto this_ty = func_ty->getParamType(0)) {
          if (this_ty == bitcast->getSrcTy()) {
            return false;
          }
        }
      }
    }
  }

  return true;
}

bool DTAResolver::heuristic_anti_contructor_vtable_pos(
    const llvm::BitCastInst *bitcast) {
  // Better heuristic than the previous one, can handle the CRTP. Based on the
  // previous one.

  if (heuristic_anti_contructor_this_type(bitcast))
    return true;

  // We know that we are in a constructor and the source type of the bitcast is
  // the same as the this argument. We then check where the bitcast is against
  // the store instruction of the vtable.
  auto struct_ty = stripPointer(bitcast->getSrcTy());
  if (struct_ty == nullptr)
    throw runtime_error(
        "struct_ty == nullptr in the heuristic_anti_contructor");

  // If it doesn't contain vtable, there is no reason to call this class in the
  // DTA graph, so no need to add it
  if (struct_ty->isStructTy()) {
    if (Resolver::TH->hasVFTable(llvm::dyn_cast<llvm::StructType>(struct_ty))) {
      return false;
    }
  }

  // So there is a vtable, the question is, where is it compared to the bitcast
  // instruction Carefull, there can be multiple vtable storage, we want to get
  // the last one vtable storage typically are : store i32 (...)** bitcast (i8**
  // getelementptr inbounds ({ [3 x i8*], [3 x i8*] }, { [3 x i8*], [3 x i8*] }*
  // @_ZTV2AA, i32 0, inrange i32 1, i32 3) to i32 (...)**), i32 (...)*** %17,
  // align 8
  // WARNING: May break when changing llvm version or using clang with version
  // > 5.0.1

  auto caller = bitcast->getFunction();
  if (caller == nullptr) {
    throw runtime_error("A bitcast instruction has no associated function");
  }

  int i = 0, vtable_num = 0, bitcast_num = 0;

  for (auto I = llvm::inst_begin(caller), E = llvm::inst_end(caller); I != E;
       ++I, ++i) {
    const auto &Inst = *I;

    if (auto store = llvm::dyn_cast<llvm::StoreInst>(&Inst)) {
      // We got a store instruction, now we are checking if it is a vtable
      // storage
      if (auto bitcast_expr =
              llvm::dyn_cast<llvm::ConstantExpr>(store->getValueOperand())) {
        if (bitcast_expr->isCast()) {
          if (auto const_gep = llvm::dyn_cast<llvm::ConstantExpr>(
                  bitcast_expr->getOperand(0))) {
            auto gep_as_inst = const_gep->getAsInstruction();
            if (auto gep =
                    llvm::dyn_cast<llvm::GetElementPtrInst>(gep_as_inst)) {
              if (auto vtable = llvm::dyn_cast<llvm::Constant>(
                      gep->getPointerOperand())) {
                // We can here assume that we found a vtable
                vtable_num = i;
              }
            }
            gep_as_inst->deleteValue();
          }
        }
      }
    }

    if (&Inst == bitcast)
      bitcast_num = i;
  }

  return (bitcast_num > vtable_num);
}

void DTAResolver::otherInst(const llvm::Instruction *Inst) {
  if (auto BitCast = llvm::dyn_cast<llvm::BitCastInst>(Inst)) {
    // We add the connection between the two types in the DTA graph
    auto src = BitCast->getSrcTy();
    auto dest = BitCast->getDestTy();

    auto src_struct_type = llvm::dyn_cast<llvm::StructType>(stripPointer(src));
    auto dest_struct_type =
        llvm::dyn_cast<llvm::StructType>(stripPointer(dest));

    if (src_struct_type && dest_struct_type &&
        heuristic_anti_contructor_vtable_pos(BitCast))
      typegraph.addLink(dest_struct_type, src_struct_type);
  }
}

set<const llvm::Function *>
DTAResolver::resolveVirtualCall(llvm::ImmutableCallSite CS) {
  set<const llvm::Function *> possible_call_targets;
  auto &lg = lg::get();

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Call virtual function: "
                << llvmIRToString(CS.getInstruction()));

  auto vtable_index = getVFTIndex(CS);
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

  auto receiver_type = getReceiverType(CS);

  auto possible_types = typegraph.getTypes(receiver_type);

  // WARNING We deactivated the check on allocated because it is
  // unabled to get the types allocated in the used libraries
  // auto allocated_types = IRDB.getAllocatedTypes();
  // auto end_it = allocated_types.end();
  for (auto possible_type : possible_types) {
    if (auto possible_type_struct =
            llvm::dyn_cast<llvm::StructType>(possible_type)) {
      // if ( allocated_types.find(possible_type_struct) != end_it ) {
      auto Target =
          getNonPureVirtualVFTEntry(possible_type_struct, vtable_index, CS);
      if (Target) {
        possible_call_targets.insert(Target);
      }
    }
  }

  if (possible_call_targets.empty())
    possible_call_targets = CHAResolver::resolveVirtualCall(CS);

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Possible targets are:");
  for (auto entry : possible_call_targets) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << entry);
  }

  return possible_call_targets;
}
