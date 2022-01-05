/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_JOINEDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_JOINEDGEFUNCTION_H

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"

namespace psr::XTaint {
class JoinEdgeFunction : public EdgeFunctionBase {
  struct EFDenseSetInfo {
    static inline EdgeFunctionPtrType getEmptyKey() {
      static EdgeFunctionPtrType EmptyKey(
          llvm::DenseMapInfo<EdgeFunction<l_t> *>::getEmptyKey(),
          [](const EdgeFunction<l_t> * /*unused*/) {});

      return EmptyKey;
    }
    static inline EdgeFunctionPtrType getTombstoneKey() {
      static EdgeFunctionPtrType TombstoneKey(
          llvm::DenseMapInfo<EdgeFunction<l_t> *>::getTombstoneKey(),
          [](const EdgeFunction<l_t> * /*unused*/) {});

      return TombstoneKey;
    }
    static unsigned getHashValue(const EdgeFunctionPtrType &EF) {
      return XTaint::getHashCode(EF);
    }
    static bool isEqual(const EdgeFunctionPtrType &LHS,
                        const EdgeFunctionPtrType &RHS) {
      if (&*LHS == &*RHS) {
        return true;
      }

      if (&*LHS == llvm::DenseMapInfo<EdgeFunction<l_t> *>::getEmptyKey() ||
          &*LHS == llvm::DenseMapInfo<EdgeFunction<l_t> *>::getTombstoneKey()) {
        return false;
      }

      if (&*RHS == llvm::DenseMapInfo<EdgeFunction<l_t> *>::getEmptyKey() ||
          &*RHS == llvm::DenseMapInfo<EdgeFunction<l_t> *>::getTombstoneKey()) {
        return false;
      }

      return LHS->equal_to(RHS);
    }
  };

  using SubEdgeFuctionsTy =
      llvm::SmallDenseSet<EdgeFunctionPtrType, 2, EFDenseSetInfo>;
  // The set of joined edge-functions. Is not empty
  SubEdgeFuctionsTy SubEF;

  // The joined edge-value of collected constants (GenEdgeFunction and
  // JoinConstEdgeFunction)
  EdgeDomain Seed;

  JoinEdgeFunction(BasicBlockOrdering &BBO,
                   std::initializer_list<EdgeFunctionPtrType> SubEF,
                   const EdgeDomain &Seed);

public:
  // FOR INTERNAL USE ONLY! USE JoiEdgeFunction::create INSTEAD
  JoinEdgeFunction(BasicBlockOrdering &BBO, SubEdgeFuctionsTy &&SubEF,
                   const EdgeDomain &Seed);
  // Replaces the constructor and aims to deduplicate the sub-edge-functions if
  // one or more of {First, Second} are also JoinEdgeFunctions
  static EdgeFunctionPtrType create(BasicBlockOrdering &BBO,
                                    EdgeFunctionPtrType First,
                                    EdgeFunctionPtrType Second);

  l_t computeTarget(l_t Source) override;

  bool equal_to(EdgeFunctionPtrType OtherFunction) const override;

  void print(std::ostream &OS, bool IsForDebug = false) const override;

  llvm::hash_code getHashCode() const override;

  static inline bool classof(const EdgeFunctionBase *EF) {
    return EF->getKind() == EFKind::Join;
  }
};
} // namespace psr::XTaint

#endif
