#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/DebugIdentityEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/JoinEdgeFunction.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace CCPP::LCUtils {
DebugIdentityEdgeFunction::DebugIdentityEdgeFunction(
    const llvm::Instruction *from, const llvm::Instruction *to, size_t maxSize)
    : from(from), to(to), maxSize(maxSize) {}

IDELinearConstantPropagation::v_t DebugIdentityEdgeFunction::computeTarget(
    IDELinearConstantPropagation::v_t source) {

  print(std::cout << "COMPUTE_TARGET: ");
  std::cout << "\twhich is " << source << std::endl;
  return source;
}

std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
DebugIdentityEdgeFunction::composeWith(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
        secondFunction) {
  print(std::cout << "COMPOSE: ");
  return std::make_shared<EdgeFunctionComposer>(shared_from_this(),
                                                secondFunction, maxSize);
}

std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
DebugIdentityEdgeFunction::joinWith(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
        otherFunction) {
  print(std::cout << "JOIN: ");
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), otherFunction,
                                            maxSize);
}

bool DebugIdentityEdgeFunction::equal_to(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> other)
    const {
  return this == other.get();
}
void DebugIdentityEdgeFunction::print(std::ostream &OS, bool isForDebug) const {
  OS << "FROM " << psr::llvmIRToString(from) << " TO "
     << psr::llvmIRToString(to) << std::endl;
}
} // namespace CCPP::LCUtils