/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_XTAINTEDGEFUNCTIONBASE_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_XTAINTEDGEFUNCTIONBASE_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/JoinLattice.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"

#include "llvm/ADT/Hashing.h"

#include <memory>

namespace psr::XTaint {

static constexpr size_t JoinThreshold = 2;

static EdgeFunction<EdgeDomain>
makeComposeEF(const EdgeFunction<EdgeDomain> &F,
              const EdgeFunction<EdgeDomain> &G);
template <typename Derived> struct EdgeFunctionBase {
  BasicBlockOrdering *BBO{};

  static EdgeFunction<EdgeDomain>
  compose(EdgeFunctionRef<Derived> This,
          const EdgeFunction<EdgeDomain> &SecondFunction) {
    if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
      return Default;
    }
    return makeComposeEF(This, SecondFunction);
  }

  static EdgeFunction<EdgeDomain>
  join(EdgeFunctionRef<Derived> This,
       const EdgeFunction<EdgeDomain> &OtherFunction) {
    if (auto Default =
            defaultJoinOrNull<EdgeDomain, JoinLatticeTraits<EdgeDomain>,
                              JoinThreshold>(This, OtherFunction)) {
      return Default;
    }
    if (OtherFunction == getAllSanitized()) {
      return This;
    }
  }

  // llvm::hash_code getHashCode() const ;
};

/// A common baseclass for all EdgeFunctions used for the
/// IDEExtendedTaintAnalysis. Implements default overrides for composeWith and
/// joinWith and additionally provides LLVM-style RTTI allowing fast runtime
/// typechecks.
class EdgeFunctionBase : public EdgeFunction<EdgeDomain>,
                         public std::enable_shared_from_this<EdgeFunctionBase> {
public:
  enum class EFKind { Gen, Join, JoinConst, Compose, KillIfSani, Transfer };

protected:
  BasicBlockOrdering &BBO;

private:
  const EFKind Kind;

public:
  using l_t = EdgeDomain;

  EdgeFunctionBase(EFKind Kind, BasicBlockOrdering &BBO);
  ~EdgeFunctionBase() override = default;

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction) override;
  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override;

  /// The actualy kind of this edge function. Can be used in a type-switch.
  [[nodiscard]] inline EFKind getKind() const { return Kind; }

  virtual llvm::hash_code getHashCode() const = 0;
};
} // namespace psr::XTaint
#endif
