/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * RTAResolver.cpp
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#include <llvm/IR/CallSite.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>

#include <phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/Utils/PAMM.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

RTAResolver::RTAResolver(ProjectIRDB &irdb, const LLVMTypeHierarchy &ch) : CHAResolver(irdb, ch);

void RTAResolver::firstFunction(const llvm::Function* F) {
  auto func_type = F->getFunctionType();

  for ( auto param : func_type->params() ) {
    if ( llvm::isa<llvm::PointerType>(param) ) {
      if ( auto struct_ty = llvm::dyn_cast<llvm::StructType>(stripPointer(param)) ) {
        unsound_types.insert(struct_ty);
      }
    }
  }
}

set<string> resolveVirtualCall(const llvm::ImmutableCallSite &CS) {}
