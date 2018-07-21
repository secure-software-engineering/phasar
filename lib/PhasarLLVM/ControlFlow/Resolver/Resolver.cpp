/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Resolver.cpp
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#include <llvm/IR/CallSite.h>
#include <llvm/IR/DerivedTypes.h>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/Utils/PAMM.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

Resolver::Resolver(ProjectIRDB &irdb, const LLVMTypeHierarchy &ch) : IRDB(irdb), CH(ch) {}

unsigned int Resolver::getVtableIndex(const llvm::ImmutableCallSite &CS) const {
  // deal with a virtual member function
  // retrieve the vtable entry that is called
  const llvm::LoadInst *load =
      llvm::dyn_cast<llvm::LoadInst>(CS.getCalledValue());

  if ( load == nullptr ) {
    return -1;
  }

  const llvm::GetElementPtrInst *gep =
      llvm::dyn_cast<llvm::GetElementPtrInst>(load->getPointerOperand());

  if ( gep == nullptr ) {
    return -2;
  }

  unsigned vtable_index =
      llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(1))->getZExtValue();

  return vtable_index;
}

std::string Resolver::getReceiverTypeName(const llvm::ImmutableCallSite &CS) const {
  const llvm::Value *receiver = CS.getArgOperand(0);
  const llvm::StructType *receiver_type = llvm::dyn_cast<llvm::StructType>(
      receiver->getType()->getPointerElementType());
  if (!receiver_type) {
    throw runtime_error("Receiver type is not a struct type!");
  }

  string receiver_type_name = receiver_type->getName().str();

  return receiver_type_name;
}

void Resolver::insertVtableIntoResult(std::set<std::string> &results, std::string &struct_name, unsigned vtable_index) {
  auto vtable_entry = CH.getVTableEntry(struct_name, vtable_index);
  if (vtable_entry != "" && vtable_entry != "__cxa_pure_virtual") {
    results.insert(vtable_entry);
  }
}

void Resolver::preCall(const llvm::Instruction* inst) {};
void Resolver::postCall(const llvm::Instruction* inst) {};
void Resolver::OtherInst(const llvm::Instruction* inst) {};

void Resolver::firstFunction(const llvm::Function* F) {}

set<string> Resolver::resolveFunctionPointer(const llvm::ImmutableCallSite &CS) {
  // We may want to optimise the time of this function as it is in fact most of the time
  // spent in the ICFG construction and it grows rapidily
  PAMM_FACTORY;
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
      << "Call function pointer: " << llvmIRToString(CS.getInstruction()));
  INC_COUNTER("ICFG function pointer calls");

  set<string> possible_call_targets;
  // *CS.getCalledValue() == nullptr* can happen in extremely rare cases (the origin is still unknown)
  if (CS.getCalledValue() != nullptr && CS.getCalledValue()->getType()->isPointerTy()) {
    if (const llvm::FunctionType *ftype = llvm::dyn_cast<llvm::FunctionType>(
            CS.getCalledValue()->getType()->getPointerElementType())) {
      for (auto f : IRDB.getAllFunctions()) {
        if (matchesSignature(f, ftype)) {
          possible_call_targets.insert(f->getName().str());
        }
      }
    }
  }

  return possible_call_targets;
}
