/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGUTILITIES_H
#define PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGUTILITIES_H

#include <algorithm>
#include <iterator>
#include <type_traits>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {
template <typename ContainerTy,
          typename = std::enable_if_t<std::is_same_v<
              typename ContainerTy::value_type, const llvm::Value *>>>
void collectGeneratedFacts(ContainerTy &Dest, const TaintConfig &Config,
                           const llvm::CallBase *CB,
                           const llvm::Function *Callee) {
  auto Callback = Config.getRegisteredSourceCallBack();
  if (Callback) {
    auto CBFacts = Callback(CB);
    Dest.insert(CBFacts.begin(), CBFacts.end());
  }

  if (Config.isSource(CB)) {
    Dest.insert(CB);
  }

  for (unsigned I = 0, End = Callee->arg_size(); I < End; ++I) {
    if (Config.isSource(Callee->getArg(I))) {
      Dest.insert(CB->getArgOperand(I));
    }
  }
}

template <typename ContainerTy, typename Pred,
          typename = std::enable_if_t<std::is_same_v<
              typename ContainerTy::value_type, const llvm::Value *>>>
void collectLeakedFacts(ContainerTy &Dest, const TaintConfig &Config,
                        const llvm::CallBase *CB, const llvm::Function *Callee,
                        Pred &&LeakIf) {

  auto Callback = Config.getRegisteredSinkCallBack();
  if (Callback) {
    auto CBLeaks = Callback(CB);
    std::copy_if(CBLeaks.begin(), CBLeaks.end(),
                 std::inserter(Dest, Dest.end()), LeakIf);
  }

  for (unsigned I = 0, End = Callee->arg_size(); I < End; ++I) {
    if (Config.isSink(Callee->getArg(I)) && LeakIf(CB->getArgOperand(I))) {
      Dest.insert(CB->getArgOperand(I));
    }
  }
}

template <typename ContainerTy>
inline void collectLeakedFacts(ContainerTy &Dest, const TaintConfig &Config,
                               const llvm::CallBase *CB,
                               const llvm::Function *Callee) {
  collectLeakedFacts(Dest, Config, CB, Callee,
                     [](const llvm::Value * /*V*/) { return true; });
}

template <typename ContainerTy,
          typename = std::enable_if_t<std::is_same_v<
              typename ContainerTy::value_type, const llvm::Value *>>>
void collectSanitizedFacts(ContainerTy &Dest, const TaintConfig &Config,
                           const llvm::CallBase *CB,
                           const llvm::Function *Callee) {
  for (unsigned I = 0, End = Callee->arg_size(); I < End; ++I) {
    if (Config.isSanitizer(Callee->getArg(I))) {
      Dest.insert(CB->getArgOperand(I));
    }
  }
}
} // namespace psr

#endif
