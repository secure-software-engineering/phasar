#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeValueSet.h"

namespace psr::LCUtils {
class TypecastEdgeFunction
    : public psr::EdgeFunction<IDELinearConstantPropagation::v_t>,
      public std::enable_shared_from_this<TypecastEdgeFunction> {
  unsigned bits;
  EdgeValue::Type dest;
  size_t maxSize;

public:
  TypecastEdgeFunction(unsigned bits, EdgeValue::Type dest, size_t maxSize)
      : bits(bits), dest(dest), maxSize(maxSize) {}

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