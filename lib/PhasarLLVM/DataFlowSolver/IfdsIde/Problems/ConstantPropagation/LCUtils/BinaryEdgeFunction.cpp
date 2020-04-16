#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/BinaryEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeFunctionComposer.h"

namespace psr::LCUtils {

IDELinearConstantPropagation::v_t
BinaryEdgeFunction::computeTarget(IDELinearConstantPropagation::v_t source) {
  /*auto ret = leftConst ? performBinOp(op, cnst, source, maxSize)
                       : performBinOp(op, source, cnst, maxSize);
  std::cout << "Binary(" << source << ") = " << ret << std::endl;
  return ret;*/
  if (leftConst) {
    return performBinOp(op, cnst, source, maxSize);
  } else {
    return performBinOp(op, source, cnst, maxSize);
  }
}

std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
BinaryEdgeFunction::composeWith(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
        secondFunction) {
  if (auto *EI =
          dynamic_cast<psr::EdgeIdentity<IDELinearConstantPropagation::v_t> *>(
              secondFunction.get())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<psr::AllBottom<IDELinearConstantPropagation::v_t> *>(
          secondFunction.get())) {
    // print(std::cout << "Compose ");
    // std::cout << " with ALLBOT" << std::endl;
    return shared_from_this();
  }
  return std::make_shared<EdgeFunctionComposer>(this->shared_from_this(),
                                                secondFunction, maxSize);
}
std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
BinaryEdgeFunction::joinWith(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
        otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<psr::AllTop<IDELinearConstantPropagation::v_t> *>(
          otherFunction.get())) {
    return this->shared_from_this();
  }
  return std::make_shared<psr::AllBottom<IDELinearConstantPropagation::v_t>>(
      IDELinearConstantPropagation::v_t({EdgeValue(nullptr)}));
}
bool BinaryEdgeFunction::equal_to(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> other)
    const {
  return this == other.get();
}
void BinaryEdgeFunction::print(std::ostream &OS, bool isForDebug) const {
  OS << "Binary_" << op;
}
} // namespace psr::LCUtils