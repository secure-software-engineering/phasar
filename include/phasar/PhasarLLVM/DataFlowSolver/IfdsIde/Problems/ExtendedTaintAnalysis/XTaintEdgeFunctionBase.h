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

#include <memory>

#include "llvm/ADT/Hashing.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"

namespace psr::XTaint {
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
