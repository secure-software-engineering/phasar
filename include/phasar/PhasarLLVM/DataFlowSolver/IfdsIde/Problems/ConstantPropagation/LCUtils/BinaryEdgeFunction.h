#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeValueSet.h"

namespace psr::LCUtils {
class BinaryEdgeFunction
    : public psr::EdgeFunction<IDELinearConstantPropagation::v_t>,
      public std::enable_shared_from_this<BinaryEdgeFunction> {
  const IDELinearConstantPropagation::v_t cnst;
  bool leftConst;
  size_t maxSize;
  llvm::BinaryOperator::BinaryOps op;

public:
  BinaryEdgeFunction(llvm::BinaryOperator::BinaryOps op,
                     const IDELinearConstantPropagation::v_t &cnst,
                     bool leftConst, size_t maxSize)
      : op(op), cnst(cnst), leftConst(leftConst), maxSize(maxSize) {}

  IDELinearConstantPropagation::v_t
  computeTarget(IDELinearConstantPropagation::v_t source) override;

  std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
  composeWith(
      std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
          secondFunction) override;

  std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
  joinWith(std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
               otherFunction) override;

  bool
  equal_to(std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
               other) const override;

  void print(std::ostream &OS, bool isForDebug = false) const override;
};
} // namespace psr::LCUtils