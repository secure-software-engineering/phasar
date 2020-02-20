/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_MAPFACTSALONGSIDECALLSITE_H_
#define PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_MAPFACTSALONGSIDECALLSITE_H_

#include <functional>
#include <set>
#include <vector>

#include <llvm/IR/CallSite.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>

namespace llvm {
class Value;
class Function;
class ImmutableCallSite;
} // namespace llvm

namespace psr {

/**
 * A predicate can be used to specify additional requirements for the
 * propagation.
 * @brief Propagates all non pointer parameters alongside the call site.
 */
class MapFactsAlongsideCallSite : public FlowFunction<const llvm::Value *> {
protected:
  llvm::ImmutableCallSite CallSite;
  std::function<bool(llvm::ImmutableCallSite, const llvm::Value *)> Predicate;

public:
  MapFactsAlongsideCallSite(
      llvm::ImmutableCallSite CallSite,
      std::function<bool(llvm::ImmutableCallSite, const llvm::Value *)>
          Predicate = [](llvm::ImmutableCallSite CS, const llvm::Value *V) {
            // Checks if a values is involved in a call, i.e. may be modified
            // by a callee, in which case its flow is controlled by
            // getCallFlowFunction() and getRetFlowFunction().
            bool Involved = false;
            for (auto &Arg : CS.args()) {
              if (Arg == V && V->getType()->isPointerTy()) {
                Involved = true;
              }
            }
            return Involved;
          });
  virtual ~MapFactsAlongsideCallSite() = default;

  std::set<const llvm::Value *>
  computeTargets(const llvm::Value *source) override;
};

} // namespace psr

#endif
