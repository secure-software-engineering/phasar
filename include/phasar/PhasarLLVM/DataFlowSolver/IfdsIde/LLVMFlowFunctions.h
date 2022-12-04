/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_LLVMFLOWFUNCTIONS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_LLVMFLOWFUNCTIONS_H

#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "llvm/ADT/PointerIntPair.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

namespace psr {

//===----------------------------------------------------------------------===//
// Mapping functions

/// A predicate can be used to specify additional requirements for the
/// propagation.
/// \brief Propagates all non pointer parameters alongside the call site.
template <
    typename Fn = TrueFn, typename Container = std::set<const llvm::Value *>,
    typename =
        std::enable_if_t<std::is_invocable_r_v<bool, Fn, const llvm::Value *>>>
auto mapFactsAlongsideCallSite(const llvm::CallBase *CallSite,
                               bool PropagateGlobals, Fn &&PropagateArgs = {}) {
  struct Mapper : public FlowFunction<const llvm::Value *, Container> {

    Mapper(const llvm::CallBase *CS, bool PropagateGlobals, Fn &&PropArgs)
        : CSAndPropGlob(CS, PropagateGlobals),
          PropArgs(std::forward<Fn>(PropArgs)) {}

    Container computeTargets(const llvm::Value *Source) override {
      // Pass ZeroValue as is
      if (LLVMZeroValue::isLLVMZeroValue(Source)) {
        return {Source};
      }
      // Pass global variables as is, if desired
      // Need llvm::Constant here to cover also ConstantExpr and
      // ConstantAggregate
      if (llvm::isa<llvm::Constant>(Source)) {
        if (CSAndPropGlob.getInt()) {
          return {Source};
        }
        return {};
      }

      for (const auto &Arg : CSAndPropGlob.getPointer()->args()) {
        if (Arg.get() == Source) {
          if (std::invoke(PropArgs, Arg.get())) {
            return {Arg.get()};
          }
          return {};
        }
      }

      return {Source};
    }

    llvm::PointerIntPair<const llvm::CallBase *, 1, bool> CSAndPropGlob;
    std::decay_t<Fn> PropArgs;
  };

  return std::make_shared<Mapper>(CallSite, PropagateGlobals,
                                  std::forward<Fn>(PropagateArgs));
}

/// A predicate can be used to specifiy additonal requirements for mapping
/// actual parameter into formal parameter.
/// \brief Generates all valid formal parameter in the callee context.
///
/// \note Unlike the old version, this one is only meant for forward-analyses
template <typename Fn = std::equal_to<const llvm::Value *>,
          typename Container = std::set<const llvm::Value *>,
          typename = std::enable_if_t<std::is_invocable_r_v<
              bool, Fn, const llvm::Value *, const llvm::Value *>>>
FlowFunctionPtrType<const llvm::Value *, Container>
mapFactsToCallee(const llvm::CallBase *CallSite, const llvm::Function *DestFun,
                 bool PropagateGlobals = true,
                 Fn &&PropagateArgumentWithSource = {},
                 bool PropagateZeroToCallee = true) {
  struct Mapper : public FlowFunction<const llvm::Value *, Container> {

    Mapper(const llvm::CallBase *CS, const llvm::Function *DestFun,
           bool PropagateGlobals, bool PropagateZeroToCallee, Fn &&PropArg)
        : CSAndPropGlob(CS, PropagateGlobals),
          DestFunAndPropZero(DestFun, PropagateZeroToCallee),
          PropArg(std::forward<Fn>(PropArg)) {}

    Container computeTargets(const llvm::Value *Source) override {
      // If DestFun is a declaration we cannot follow this call, we thus need to
      // kill everything
      if (DestFunAndPropZero.getPointer()->isDeclaration()) {
        return {};
      }

      Container Res;
      if (DestFunAndPropZero.getInt() &&
          LLVMZeroValue::isLLVMZeroValue(Source)) {
        Res.insert(Source);
      } else if (CSAndPropGlob.getInt() &&
                 !LLVMZeroValue::isLLVMZeroValue(Source) &&
                 llvm::isa<llvm::Constant>(Source)) {
        // Pass global variables as is, if desired
        // Globals could also be actual arguments, then the formal argument
        // needs to be generated below. Need llvm::Constant here to cover also
        // ConstantExpr and ConstantAggregate
        Res.insert(Source);
      }

      const auto *CS = CSAndPropGlob.getPointer();
      const auto *DestFun = DestFunAndPropZero.getPointer();
      assert(CS->arg_size() >= DestFun->arg_size());
      assert(CS->arg_size() == DestFun->arg_size() || DestFun->isVarArg());

      llvm::CallBase::const_op_iterator ArgIt = CS->arg_begin();
      llvm::CallBase::const_op_iterator ArgEnd = CS->arg_end();
      llvm::Function::const_arg_iterator ParamIt = DestFun->arg_begin();
      llvm::Function::const_arg_iterator ParamEnd = DestFun->arg_end();

      for (; ParamIt != ParamEnd; ++ParamIt, ++ArgIt) {
        if (std::invoke(PropArg, ArgIt->get(), Source)) {
          Res.insert(&*ParamIt);
        }
      }

      if (ArgIt != ArgEnd &&
          std::any_of(
              ArgIt, ArgEnd,
              std::bind(std::ref(PropArg), std::placeholders::_1, Source))) {
        // Over-approximate by trying to add the
        //   alloca [1 x %struct.__va_list_tag], align 16
        // to the results
        // find the allocated %struct.__va_list_tag and generate it

        for (const auto &BB : *DestFun) {
          for (const auto &I : BB) {
            if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
              if (Alloc->getAllocatedType()->isArrayTy() &&
                  Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
                  Alloc->getAllocatedType()
                      ->getArrayElementType()
                      ->isStructTy() &&
                  Alloc->getAllocatedType()
                          ->getArrayElementType()
                          ->getStructName() == "struct.__va_list_tag") {
                Res.insert(Alloc);
              }
            }
          }
        }
      }

      return Res;
    }

    llvm::PointerIntPair<const llvm::CallBase *, 1, bool> CSAndPropGlob;
    llvm::PointerIntPair<const llvm::Function *, 1, bool> DestFunAndPropZero;
    std::decay_t<Fn> PropArg;
  };

  return std::make_shared<Mapper>(
      CallSite, DestFun, PropagateGlobals, PropagateZeroToCallee,
      std::forward<Fn>(PropagateArgumentWithSource));
}

/// Predicates can be used to specify additional requirements for mapping
/// actual parameters into formal parameters and the return value.
/// \note Currently, the return value predicate only allows checks regarding
/// the callee method.
/// \brief Generates all valid actual parameters and the return value in the
/// caller context.
template <typename FnParam = std::equal_to<const llvm::Value *>,
          typename Container = std::set<const llvm::Value *>,
          typename = std::enable_if_t<std::is_invocable_r_v<
              bool, FnParam, const llvm::Value *, const llvm::Value *>>>
FlowFunctionPtrType<const llvm::Value *, Container> mapFactsToCaller(
    const llvm::CallBase *CallSite, const llvm::Instruction *ExitInst,
    bool PropagateGlobals = true, FnParam &&PropagateParameter = {},
    bool PropagateZeroToCaller = true) {
  struct Mapper : public FlowFunction<const llvm::Value *, Container> {
    Mapper(const llvm::CallBase *CallSite, const llvm::Instruction *ExitInst,
           bool PropagateGlobals, FnParam &&PropagateParameter,
           bool PropagateZeroToCaller)
        : CSAndPropGlob(CallSite, PropagateGlobals),
          ExitInstAndPropZero(ExitInst, PropagateZeroToCaller),
          PropArg(std::forward<FnParam>(PropagateParameter)) {}

    Container computeTargets(const llvm::Value *Source) override {
      Container Res;
      if (ExitInstAndPropZero.getInt() &&
          LLVMZeroValue::isLLVMZeroValue(Source)) {
        Res.insert(Source);
      } else if (CSAndPropGlob.getInt() && llvm::isa<llvm::Constant>(Source)) {
        // Pass global variables as is, if desired
        // Globals could also be actual arguments, then the formal argument
        // needs to be generated below. Need llvm::Constant here to cover also
        // ConstantExpr and ConstantAggregate
        Res.insert(Source);
      }

      const auto *CS = CSAndPropGlob.getPointer();
      const auto *DestFun = ExitInstAndPropZero.getPointer()->getFunction();
      assert(CS->arg_size() >= DestFun->arg_size());
      assert(CS->arg_size() == DestFun->arg_size() || DestFun->isVarArg());

      llvm::CallBase::const_op_iterator ArgIt = CS->arg_begin();
      llvm::CallBase::const_op_iterator ArgEnd = CS->arg_end();
      llvm::Function::const_arg_iterator ParamIt = DestFun->arg_begin();
      llvm::Function::const_arg_iterator ParamEnd = DestFun->arg_end();

      for (; ParamIt != ParamEnd; ++ParamIt, ++ArgIt) {
        if (std::invoke(PropArg, &*ParamIt, Source)) {
          Res.insert(ArgIt->get());
        }
      }

      if (ArgIt != ArgEnd) {
        // Over-approximate by trying to add the
        //   alloca [1 x %struct.__va_list_tag], align 16
        // to the results
        // find the allocated %struct.__va_list_tag and generate it

        for (const auto &I : llvm::instructions(DestFun)) {
          if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
            const auto *AllocTy = Alloc->getAllocatedType();
            if (AllocTy->isArrayTy() && AllocTy->getArrayNumElements() > 0 &&
                AllocTy->getArrayElementType()->isStructTy() &&
                AllocTy->getArrayElementType()->getStructName() ==
                    "struct.__va_list_tag") {
              if (std::invoke(PropArg, Alloc, Source)) {
                Res.insert(ArgIt, ArgEnd);
                break;
              }
            }
          }
        }
      }

      if (const auto *RetInst = llvm::dyn_cast<llvm::ReturnInst>(
              ExitInstAndPropZero.getPointer());
          RetInst && RetInst->getReturnValue()) {
        if (std::invoke(PropArg, RetInst->getReturnValue(), Source)) {
          Res.insert(CS);
        }
      }

      return Res;
    }

    llvm::PointerIntPair<const llvm::CallBase *, 1, bool> CSAndPropGlob;
    llvm::PointerIntPair<const llvm::Instruction *, 1, bool>
        ExitInstAndPropZero;
    std::decay_t<FnParam> PropArg;
  };

  return std::make_shared<Mapper>(CallSite, ExitInst, PropagateGlobals,
                                  std::forward<FnParam>(PropagateParameter),
                                  PropagateZeroToCaller);
}

//===----------------------------------------------------------------------===//
// Propagation flow functions

template <typename Container = std::set<const llvm::Value *>>
FlowFunctionPtrType<const llvm::Value *, Container>
propagateLoad(const llvm::LoadInst *Load) {
  return generateFlow<const llvm::Value *, Container>(
      Load, Load->getPointerOperand());
}
template <typename Container = std::set<const llvm::Value *>>
FlowFunctionPtrType<const llvm::Value *, Container>
propagateStore(const llvm::StoreInst *Store) {
  return generateFlow<const llvm::Value *, Container>(
      Store->getValueOperand(), Store->getPointerOperand());
}

//===----------------------------------------------------------------------===//
// Update flow functions

template <typename Fn, typename Container = std::set<const llvm::Value *>,
          typename = std::enable_if_t<
              std::is_invocable_r_v<bool, Fn, const llvm::Value *>>>
FlowFunctionPtrType<const llvm::Value *, Container>
strongUpdateStore(const llvm::StoreInst *Store, Fn &&GeneratePointerOpIf) {
  struct StrongUpdateFlow
      : public FlowFunction<const llvm::Value *, Container> {

    StrongUpdateFlow(const llvm::StoreInst *Store, Fn &&GeneratePointerOpIf)
        : Store(Store), Pred(std::forward<Fn>(GeneratePointerOpIf)) {}

    Container computeTargets(const llvm::Value *Source) override {
      if (Source == Store->getPointerOperand()) {
        return {};
      }
      if (std::invoke(Pred, Source)) {
        return {Source, Store->getPointerOperand()};
      }
      return {Source};
    }

    const llvm::StoreInst *Store;
    std::decay_t<Fn> Pred;
  };

  return std::make_shared<StrongUpdateFlow>(
      Store, std::forward<Fn>(GeneratePointerOpIf));
}

template <typename Container = std::set<const llvm::Value *>>
FlowFunctionPtrType<const llvm::Value *, Container>
strongUpdateStore(const llvm::StoreInst *Store) {
  struct StrongUpdateFlow
      : public FlowFunction<const llvm::Value *, Container> {

    StrongUpdateFlow(const llvm::StoreInst *Store) : Store(Store) {}

    Container computeTargets(const llvm::Value *Source) override {
      if (Source == Store->getPointerOperand()) {
        return {};
      }
      if (Source == Store->getValueOperand()) {
        return {Source, Store->getPointerOperand()};
      }
      return {Source};
    }

    const llvm::StoreInst *Store;
  };

  return std::make_shared<StrongUpdateFlow>(Store);
}

} // namespace psr

#endif
