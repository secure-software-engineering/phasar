#pragma once
#include <memory>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeValueSet.h"

namespace CCPP::LCUtils {
class DebugIdentityEdgeFunction
    : public psr::EdgeFunction<IDELinearConstantPropagation::v_t>,
      public std::enable_shared_from_this<DebugIdentityEdgeFunction> {
  const llvm::Instruction *from, *to;
  size_t maxSize;

public:
  DebugIdentityEdgeFunction(const llvm::Instruction *from,
                            const llvm::Instruction *to, size_t maxSize);
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
} // namespace CCPP::LCUtils