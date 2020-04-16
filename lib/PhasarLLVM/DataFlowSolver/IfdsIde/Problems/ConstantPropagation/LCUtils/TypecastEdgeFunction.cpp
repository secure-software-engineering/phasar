#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/TypecastEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/JoinEdgeFunction.h"

namespace CCPP::LCUtils {

IDELinearConstantPropagation::v_t
TypecastEdgeFunction::computeTarget(IDELinearConstantPropagation::v_t source) {
  return performTypecast(source, dest, bits);
}

std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
TypecastEdgeFunction::composeWith(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
        secondFunction) {
  if (dynamic_cast<psr::AllBottom<IDELinearConstantPropagation::v_t> *>(
          secondFunction.get()))
    return shared_from_this();
  return std::make_shared<EdgeFunctionComposer>(shared_from_this(),
                                                secondFunction, maxSize);
}

std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
TypecastEdgeFunction::joinWith(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
        otherFunction) {
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), otherFunction,
                                            maxSize);
}

bool TypecastEdgeFunction::equal_to(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> other)
    const {
  if (this == other.get())
    return true;
  if (auto otherTC = dynamic_cast<TypecastEdgeFunction *>(other.get())) {
    return bits == otherTC->bits && dest == otherTC->dest;
  }
  return false;
}

void TypecastEdgeFunction::print(std::ostream &OS, bool isForDebug) const {
  OS << "TypecastEdgeFn[to=" << EdgeValue::typeToString(dest)
     << "; bits=" << bits << "]";
}
} // namespace CCPP::LCUtils