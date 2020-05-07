/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann and others
 *****************************************************************************/

#include <algorithm>
#include <ostream>
#include <unordered_map>
#include <utility>

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

namespace psr {

InterMonoFullConstantPropagation::InterMonoFullConstantPropagation(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    const std::set<std::string> &EntryPoints)
    : InterMonoProblem<InterMonoFullConstantPropagation::n_t,
                       InterMonoFullConstantPropagation::d_t,
                       InterMonoFullConstantPropagation::f_t,
                       InterMonoFullConstantPropagation::t_t,
                       InterMonoFullConstantPropagation::v_t,
                       InterMonoFullConstantPropagation::i_t,
                       InterMonoFullConstantPropagation::container_t>(IRDB, TH, ICF, PT,
                                                              EntryPoints),
      IntraMonoFullConstantPropagation(IRDB, TH, ICF, PT, EntryPoints) {}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::merge(
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Rhs) {
  return IntraMonoFullConstantPropagation::merge(Lhs, Rhs);
}

bool InterMonoFullConstantPropagation::equal_to(
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Lhs,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &Rhs) {
  return IntraMonoFullConstantPropagation::equal_to(Lhs, Rhs);
}

std::unordered_map<InterMonoFullConstantPropagation::n_t,
                   BitVectorSet<InterMonoFullConstantPropagation::d_t>>
InterMonoFullConstantPropagation::initialSeeds() {
  return IntraMonoFullConstantPropagation::initialSeeds();
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::normalFlow(
    InterMonoFullConstantPropagation::n_t S,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &In) {
  return IntraMonoFullConstantPropagation::normalFlow(S, In);
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::callFlow(
    InterMonoFullConstantPropagation::n_t CallSite,
    InterMonoFullConstantPropagation::f_t Callee,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &In) {
  auto Out = In;

  // Map the actual parameters into the formal parameters
  if (llvm::isa<llvm::CallInst>(CallSite) ||
      llvm::isa<llvm::InvokeInst>(CallSite)) {
    Out.clear();
    vector<const llvm::Value *> actuals;
    vector<const llvm::Value *> formals;
    auto IM = llvm::ImmutableCallSite(CallSite);
    // Set up the actual parameters
    std::cout << "actuals:\n";
    for (unsigned idx = 0; idx < IM.getNumArgOperands(); ++idx) {
      actuals.push_back(IM.getArgOperand(idx));
      std::cout << llvmIRToString(IM.getArgOperand(idx)) << "\n";
    }
    // Set up the formal parameters
    std::cout << "formals:\n";
    for (unsigned idx = 0; idx < Callee->arg_size(); ++idx) {
      formals.push_back(getNthFunctionArgument(Callee, idx));
      std::cout << llvmIRToString(getNthFunctionArgument(Callee, idx)) << "\n";
    }
    for (unsigned idx = 0; idx < actuals.size(); ++idx) {
      // Check for C-style varargs: idx >= destFun->arg_size()
      if (idx >= Callee->arg_size() && !Callee->isDeclaration()) {
        // Handle C-style varargs
        /* // Over-approximate by trying to add the
        //   alloca [1 x %struct.__va_list_tag], align 16
        // to the results
        // find the allocated %struct.__va_list_tag and generate it
        for (auto &BB : *Callee) {
          for (auto &I : BB) {
            if (auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
              if (Alloc->getAllocatedType()->isArrayTy() &&
                  Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
                  Alloc->getAllocatedType()
                      ->getArrayElementType()
                      ->isStructTy() &&
                  Alloc->getAllocatedType()
                          ->getArrayElementType()
                          ->getStructName() == "struct.__va_list_tag") {
                res.insert(Alloc);
              }
            }
          }
        } */
      } else {
        // Ordinary case: Just perform mapping
        for (const auto &elem : In) {
          if (elem.first == actuals[idx]) {
            Out.insert({formals[idx], elem.second}); // corresponding formal
            break;
          }
        }
      }
      // Special case: Check if function is called with integer literals as
      // parameter (in case of varargs ignore)
      if (idx < Callee->arg_size() &&
          llvm::isa<llvm::ConstantInt>(actuals[idx])) {
        auto val = llvm::dyn_cast<llvm::ConstantInt>(actuals[idx]);
        Out.insert({formals[idx], val->getSExtValue()}); // corresponding formal
      }
    }
    // TODO: Handle globals
    /*
    if (llvm::isa<llvm::GlobalVariable>(source)) {
      res.insert(source);
    }*/

    return Out;
  }
  // Pass everything else as identity
  return In;
} // namespace psr

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::returnFlow(
    InterMonoFullConstantPropagation::n_t CallSite,
    InterMonoFullConstantPropagation::f_t Callee,
    InterMonoFullConstantPropagation::n_t ExitStmt,
    InterMonoFullConstantPropagation::n_t RetSite,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &In) {
  auto Out = In;

  if (CallSite->getType()->isIntegerTy()) {
    auto Return = llvm::dyn_cast<llvm::ReturnInst>(ExitStmt);
    auto ReturnValue = Return->getReturnValue();

    // Kill everything that is not returned
    Out.clear();

    // Return value is integer literal
    if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(ReturnValue)) {
      Out.insert({CallSite, CI->getSExtValue()});
      return Out;
    }

    // handle return of integer variable
    if (ReturnValue->getType()->isIntegerTy()) {
      LatticeDomain<InterMonoFullConstantPropagation::plain_d_t> latticeVal =
          Top{};
      for (const auto &elem : In) {
        if (elem.first == ReturnValue) {
          latticeVal = elem.second;
          break;
        }
      }
      if (!std::holds_alternative<Top>(latticeVal)) {
        Out.insert({CallSite, latticeVal});
      }
      return Out;
    }

    // handle Global Variables
    // TODO:handle globals
  }
  Out.clear();
  return Out;
}

BitVectorSet<InterMonoFullConstantPropagation::d_t>
InterMonoFullConstantPropagation::callToRetFlow(
    InterMonoFullConstantPropagation::n_t CallSite,
    InterMonoFullConstantPropagation::n_t RetSite,
    std::set<InterMonoFullConstantPropagation::f_t> Callees,
    const BitVectorSet<InterMonoFullConstantPropagation::d_t> &In) {
  // TODO implement
  return In;
}

void InterMonoFullConstantPropagation::printNode(
    std::ostream &os, InterMonoFullConstantPropagation::n_t n) const {
  IntraMonoFullConstantPropagation::printNode(os, n);
}

void InterMonoFullConstantPropagation::printDataFlowFact(
    std::ostream &os, InterMonoFullConstantPropagation::d_t d) const {
  IntraMonoFullConstantPropagation::printDataFlowFact(os, d);
}

void InterMonoFullConstantPropagation::printFunction(
    std::ostream &os, InterMonoFullConstantPropagation::f_t f) const {
  IntraMonoFullConstantPropagation::printFunction(os, f);
}

} // namespace psr
