#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/AllBot.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/GenConstant.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IdentityEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/JoinEdgeFunction.h"

namespace psr {

/*IDEGeneralizedLCA::v_t
EdgeFunctionComposer::computeTarget(IDEGeneralizedLCA::v_t source) {
  auto ret = this->EdgeFunctionComposer<
      IDEGeneralizedLCA::v_t>::computeTarget(source);
  std::cout << "Compose(" << source << ") = " << ret << std::endl;
  return ret;
}*/
EdgeFunctionComposer::EdgeFunctionComposer(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> F,
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> G,
    size_t maxSize)
    : EdgeFunctionComposer<IDEGeneralizedLCA::v_t>(F, G),
      maxSize(maxSize) {}
std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
EdgeFunctionComposer::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
        secondFunction) {
  // see <phasar/PhasarLVM/IfdsIde/IDELinearConstantAnalysis.h>

  if (dynamic_cast<EdgeIdentity<IDEGeneralizedLCA::v_t> *>(
          secondFunction.get()) ||
      dynamic_cast<AllBottom<IDEGeneralizedLCA::v_t> *>(
          secondFunction.get())) {
    return shared_from_this();
  }
  if (dynamic_cast<IdentityEdgeFunction *>(secondFunction.get())) {
    return shared_from_this();
  }
  if (dynamic_cast<GenConstant *>(secondFunction.get())) {
    return secondFunction;
  }
  auto gPrime = G->composeWith(secondFunction);
  if (gPrime->equal_to(G)) {
    return shared_from_this();
  }
  return F->composeWith(gPrime);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
EdgeFunctionComposer::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
        otherFunction) {
  // see <phasar/PhasarLVM/IfdsIde/IDELinearConstantAnalysis.h>
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT = dynamic_cast<AllTop<IDEGeneralizedLCA::v_t> *>(
          otherFunction.get())) {
    return this->shared_from_this();
  }
  if (AllBot::isBot(otherFunction)) {
    return AllBot::getInstance();
  }
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), otherFunction,
                                            maxSize);
}
const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> &
EdgeFunctionComposer::getFirst() const {
  return F;
}
const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> &
EdgeFunctionComposer::getSecond() const {
  return G;
}

} // namespace psr
