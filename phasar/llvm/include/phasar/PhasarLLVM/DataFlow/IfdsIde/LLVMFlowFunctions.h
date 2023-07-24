/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMFLOWFUNCTIONS_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMFLOWFUNCTIONS_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/PointerIntPair.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace psr {

//===----------------------------------------------------------------------===//
// Mapping functions

/// A flow function that serves as default-implementation for the call-to-return
/// flow function.
/// For more details on the use-case of call-to-return flow functions, see the
/// documentation of FlowFunction::getCallToRetFlowFunction().
///
/// Propagates all dataflow facts unchanged, except for global variables and
/// arguments of the specified call-site.
/// Global variables are propagated only if PropagateGlobals is set to true; for
/// the argument values the PropagateArgs function defines on a per-arg basis
/// whether the respective argument should be propagated or killed.
/// By default (when not specifying PropagateArgs), all parameters are
/// propagated unchanged.
///
/// For analyses that propagate values via reference parameters in the
/// return flow function, it is useful to kill the respective arguments here to
/// enable strong updates.
///
template <typename D = const llvm::Value *, typename Container = std::set<D>,
          typename Fn = TrueFn, typename DCtor = DefaultConstruct<D>,
          typename = std::enable_if_t<
              std::is_invocable_r_v<bool, Fn, const llvm::Value *>>>
auto mapFactsAlongsideCallSite(const llvm::CallBase *CallSite,
                               Fn &&PropagateArgs = {},
                               bool PropagateGlobals = true,
                               DCtor &&FactConstructor = {}) {
  struct Mapper : public FlowFunction<D, Container> {

    Mapper(const llvm::CallBase *CS, bool PropagateGlobals, Fn &&PropArgs,
           DCtor &&FactConstructor)
        : CSAndPropGlob(CS, PropagateGlobals),
          PropArgs(std::forward<Fn>(PropArgs)),
          FactConstructor(std::forward<DCtor>(FactConstructor)) {}

    Container computeTargets(D Source) override {
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
            return {std::invoke(FactConstructor, Arg.get())};
          }
          return {};
        }
      }

      return {Source};
    }

    llvm::PointerIntPair<const llvm::CallBase *, 1, bool> CSAndPropGlob;
    [[no_unique_address]] std::decay_t<Fn> PropArgs;
    [[no_unique_address]] std::decay_t<DCtor> FactConstructor;
  };

  return std::make_shared<Mapper>(CallSite, PropagateGlobals,
                                  std::forward<Fn>(PropagateArgs),
                                  std::forward<DCtor>(FactConstructor));
}

/// A flow function that serves as default implementation of the
/// call flow function. For more information about call flow functions, see
/// FlowFunctions::getCallFlowFunction().
///
/// Propagates the arguments of the specified call-site into the callee function
/// if the function PropagateArgumentWithSource evaluates to true when invoked
/// with the argument and a currently holding dataflow fact.
/// Global variables are propagated into the callee function only, if the flag
/// PropagateGlobals is set to true.
/// The special zero (Λ) value gets propagated into the callee function if
/// PropagateZeroToCallee is true. For most analyses it makes sense to propagate
/// Λ everywhere.
///
/// Given a call-site cs: r = fun(..., ax, ...) and a function prototype
/// fun(..., px, ...). Further let f = mapFactsToCallee(cs, fun, ...). Then for
/// any dataflow fact x:
///   f(Λ)  = {Λ}  if PropagateZeroToCallee else {},
///   f(ax) = {px} if PropagateArgumentWithSource(ax, ax) else {},
///   f(g)  = {g}  if g is GlobalVariable && PropagateGlobals else {},
///   f(x)  = {px} if PropagateArgumentWithSource(ax, x) else {}.
///
/// \note Unlike the old version, this one is only meant for forward-analyses
template <typename D = const llvm::Value *, typename Container = std::set<D>,
          typename Fn = std::equal_to<D>, typename DCtor = DefaultConstruct<D>,
          typename = std::enable_if_t<
              std::is_invocable_r_v<bool, Fn, const llvm::Value *, D>>>
FlowFunctionPtrType<D, Container>
mapFactsToCallee(const llvm::CallBase *CallSite, const llvm::Function *DestFun,
                 Fn &&PropagateArgumentWithSource = {},
                 DCtor &&FactConstructor = {}, bool PropagateGlobals = true,
                 bool PropagateZeroToCallee = true) {
  struct Mapper : public FlowFunction<D, Container> {

    Mapper(const llvm::CallBase *CS, const llvm::Function *DestFun,
           bool PropagateGlobals, bool PropagateZeroToCallee, Fn &&PropArg,
           DCtor &&FactConstructor)
        : CSAndPropGlob(CS, PropagateGlobals),
          DestFunAndPropZero(DestFun, PropagateZeroToCallee),
          PropArg(std::forward<Fn>(PropArg)),
          FactConstructor(std::forward<DCtor>(FactConstructor)) {}

    Container computeTargets(D Source) override {
      // If DestFun is a declaration we cannot follow this call, we thus need to
      // kill everything
      if (DestFunAndPropZero.getPointer()->isDeclaration()) {
        return {};
      }

      Container Res;
      if (DestFunAndPropZero.getInt() &&
          LLVMZeroValue::isLLVMZeroValue(Source)) {
        Res.insert(std::move(Source));
      } else if (CSAndPropGlob.getInt() &&
                 !LLVMZeroValue::isLLVMZeroValue(Source) &&
                 llvm::isa<llvm::Constant>(Source)) {
        // Pass global variables as is, if desired
        // Globals could also be actual arguments, then the formal argument
        // needs to be generated below. Need llvm::Constant here to cover also
        // ConstantExpr and ConstantAggregate
        Res.insert(std::move(Source));
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
          Res.insert(std::invoke(FactConstructor, &*ParamIt));
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
                Res.insert(std::invoke(FactConstructor, Alloc));
              }
            }
          }
        }
      }

      return Res;
    }

    llvm::PointerIntPair<const llvm::CallBase *, 1, bool> CSAndPropGlob;
    llvm::PointerIntPair<const llvm::Function *, 1, bool> DestFunAndPropZero;
    [[no_unique_address]] std::decay_t<Fn> PropArg;
    [[no_unique_address]] std::decay_t<DCtor> FactConstructor;
  };

  return std::make_shared<Mapper>(CallSite, DestFun, PropagateGlobals,
                                  PropagateZeroToCallee,
                                  std::forward<Fn>(PropagateArgumentWithSource),
                                  std::forward<DCtor>(FactConstructor));
}

/// A flow function that serves as default-implementation of the return flow
/// function. For more information about return flow functions, see
/// FlowFunctions::getRetFlowFunction().
///
/// Propagates the return value back to the call-site and based on the
/// PropagateParameter predicate propagates back parameters holding as dataflow
/// facts.
///
/// Let a call-site cs: r = fun(..., ax, ...) a function prototype fun(...,
/// px, ...) and an exit statement exit: return rv.
/// Further given a flow function f = mapFactsToCaller(cs, exit, ...). Then for
/// all dataflow facts x holding at exit:
///   f(rv) = {r}   if PropagateRet(rv, rv) else {},
///   f(px) = {ax}  if PropagateParameter(px, px) else {},
///   f(g)  = {g}   if PropagateGlobals else {},
///   f(Λ)  = {Λ}   if PropagateZeroToCaller else {},
///   f(x)  = ({ax} if PropagateParameter(ax, x) else {}) union ({r} if
///                    PropagateRet(rv, x) else {}).
///
template <typename D = const llvm::Value *, typename Container = std::set<D>,
          typename FnParam = std::equal_to<D>,
          typename FnRet = std::equal_to<D>,
          typename DCtor = DefaultConstruct<D>,
          typename = std::enable_if_t<
              std::is_invocable_r_v<bool, FnParam, const llvm::Value *, D> &&
              std::is_invocable_r_v<bool, FnRet, const llvm::Value *, D>>>
FlowFunctionPtrType<D, Container>
mapFactsToCaller(const llvm::CallBase *CallSite,
                 const llvm::Instruction *ExitInst,
                 FnParam &&PropagateParameter = {}, FnRet &&PropagateRet = {},
                 DCtor &&FactConstructor = {}, bool PropagateGlobals = true,
                 bool PropagateZeroToCaller = true) {

  struct Mapper : public FlowFunction<D, Container> {
    Mapper(const llvm::CallBase *CallSite, const llvm::Instruction *ExitInst,
           bool PropagateGlobals, FnParam &&PropagateParameter,
           FnRet &&PropagateRet, DCtor &&FactConstructor,
           bool PropagateZeroToCaller)
        : CSAndPropGlob(CallSite, PropagateGlobals),
          ExitInstAndPropZero(ExitInst, PropagateZeroToCaller),
          PropArg(std::forward<FnParam>(PropagateParameter)),
          PropRet(std::forward<FnRet>(PropagateRet)),
          FactConstructor(std::forward<DCtor>(FactConstructor)) {}

    Container computeTargets(D Source) override {
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
          Res.insert(std::invoke(FactConstructor, ArgIt->get()));
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
                std::transform(ArgIt, ArgEnd, std::inserter(Res, Res.end()),
                               FactConstructor);
                break;
              }
            }
          }
        }
      }

      if (const auto *RetInst = llvm::dyn_cast<llvm::ReturnInst>(
              ExitInstAndPropZero.getPointer());
          RetInst && RetInst->getReturnValue()) {
        if (std::invoke(PropRet, RetInst->getReturnValue(), Source)) {
          Res.insert(std::invoke(FactConstructor, CS));
        }
      }

      return Res;
    }

    llvm::PointerIntPair<const llvm::CallBase *, 1, bool> CSAndPropGlob;
    llvm::PointerIntPair<const llvm::Instruction *, 1, bool>
        ExitInstAndPropZero;
    [[no_unique_address]] std::decay_t<FnParam> PropArg;
    [[no_unique_address]] std::decay_t<FnRet> PropRet;
    [[no_unique_address]] std::decay_t<DCtor> FactConstructor;
  };

  return std::make_shared<Mapper>(CallSite, ExitInst, PropagateGlobals,
                                  std::forward<FnParam>(PropagateParameter),
                                  std::forward<FnRet>(PropagateRet),
                                  std::forward<DCtor>(FactConstructor),
                                  PropagateZeroToCaller);
}

//===----------------------------------------------------------------------===//
// Propagation flow functions

/// Utility function to simplify writing a flow function of the form:
/// generateFlow(Load, from: Load->getPointerOperand()).
template <typename Container = std::set<const llvm::Value *>>
FlowFunctionPtrType<const llvm::Value *, Container>
propagateLoad(const llvm::LoadInst *Load) {
  return generateFlow<const llvm::Value *, Container>(
      Load, Load->getPointerOperand());
}

/// Utility function to simplify writing a flow function of the form:
/// generateFlow(Store->getValueOperand(), from: Store->getPointerOperand()).
template <typename Container = std::set<const llvm::Value *>>
FlowFunctionPtrType<const llvm::Value *, Container>
propagateStore(const llvm::StoreInst *Store) {
  return generateFlow<const llvm::Value *, Container>(
      Store->getValueOperand(), Store->getPointerOperand());
}

//===----------------------------------------------------------------------===//
// Update flow functions

/// A flow function that models a strong update on a memory location modified by
/// a store instruction
///
/// Given a flow function f = strongUpdateStore(store a to b, pred), for all
/// holding dataflow facts x:
///   f(b) = {},
///   f(x) = {x, b} if pred(x) else {x}.
///
template <typename Fn, typename Container = std::set<const llvm::Value *>,
          typename = std::enable_if_t<
              std::is_invocable_r_v<bool, Fn, const llvm::Value *>>>
FlowFunctionPtrType<const llvm::Value *, Container>
strongUpdateStore(const llvm::StoreInst *Store, Fn &&GeneratePointerOpIf) {
  // Here we cheat a bit and "look through" the GetElementPtrInst to the
  // targeted memory location.
  const auto *BasePtrOp = Store->getPointerOperand()->stripPointerCasts();
  if (BasePtrOp != Store->getPointerOperand()) {
    struct StrongUpdateFlow
        : public FlowFunction<const llvm::Value *, Container> {

      StrongUpdateFlow(const llvm::Value *PointerOp,
                       const llvm::Value *BasePtrOp, Fn &&GeneratePointerOpIf)
          : PointerOp(PointerOp), BasePtrOp(BasePtrOp),
            Pred(std::forward<Fn>(GeneratePointerOpIf)) {}

      Container computeTargets(const llvm::Value *Source) override {
        if (Source == PointerOp || Source == BasePtrOp) {
          return {};
        }
        if (std::invoke(Pred, Source)) {
          return {Source, PointerOp, BasePtrOp};
        }
        return {Source};
      }

      const llvm::Value *PointerOp;
      const llvm::Value *BasePtrOp;
      [[no_unique_address]] std::decay_t<Fn> Pred;
    };

    return std::make_shared<StrongUpdateFlow>(
        Store->getPointerOperand(), BasePtrOp,
        std::forward<Fn>(GeneratePointerOpIf));
  }

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
    [[no_unique_address]] std::decay_t<Fn> Pred;
  };

  return std::make_shared<StrongUpdateFlow>(
      Store, std::forward<Fn>(GeneratePointerOpIf));
}

/// A flow function that models a strong update on a memory location modified by
/// a store instruction. Similar to transferFlow.
///
/// Given a flow function f = strongUpdateStore(store a to b), for all
/// holding dataflow facts x:
///   f(b) = {},
///   f(a) = {a, b},
///   f(x) = {x}.
///
/// In the exploded supergraph it may look as follows:
///
///               x   a   b  ...
///               |   |\  |
///               |   | \    ...
///  store a to b |   |  \   ...
///               v   v   v
///               x   a   b  ...
///
template <typename Container = std::set<const llvm::Value *>>
FlowFunctionPtrType<const llvm::Value *, Container>
strongUpdateStore(const llvm::StoreInst *Store) {
  // Here we cheat a bit and "look through" the GetElementPtrInst to the
  // targeted memory location.
  const auto *BasePtrOp = Store->getPointerOperand()->stripPointerCasts();
  if (BasePtrOp != Store->getPointerOperand()) {
    struct StrongUpdateFlow
        : public FlowFunction<const llvm::Value *, Container> {

      StrongUpdateFlow(const llvm::StoreInst *Store,
                       const llvm::Value *BasePtrOp) noexcept
          : Store(Store), BasePtrOp(BasePtrOp) {}

      Container computeTargets(const llvm::Value *Source) override {
        if (Source == Store->getPointerOperand() || Source == BasePtrOp) {
          return {};
        }
        if (Source == Store->getValueOperand()) {
          return {Source, Store->getPointerOperand(), BasePtrOp};
        }
        return {Source};
      }

      const llvm::StoreInst *Store;
      const llvm::Value *BasePtrOp;
    };

    return std::make_shared<StrongUpdateFlow>(Store, BasePtrOp);
  }

  struct StrongUpdateFlow
      : public FlowFunction<const llvm::Value *, Container> {

    StrongUpdateFlow(const llvm::StoreInst *Store) noexcept : Store(Store) {}

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
