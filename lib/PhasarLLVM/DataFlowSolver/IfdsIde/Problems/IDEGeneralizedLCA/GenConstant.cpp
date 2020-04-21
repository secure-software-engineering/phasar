#include <iostream>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/AllBot.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/GenConstant.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IdentityEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/JoinEdgeFunction.h"

namespace psr {

GenConstant::GenConstant(const IDEGeneralizedLCA::v_t &val, size_t maxSize)
    : val(val), maxSize(maxSize) {
  /*std::cout << "GenConstant: {";
  bool frst = true;
  for (auto &elem : val) {
    if (frst)
      frst = false;
    else
      std::cout << ", ";
    std::cout << elem;
  }
  std::cout << "}" << std::endl;*/
}
IDEGeneralizedLCA::v_t
GenConstant::computeTarget(IDEGeneralizedLCA::v_t source) {
  // std::cout << "GenConstant computation (" << source << ")"
  //         << " = " << val << std::endl;
  return val;
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> GenConstant::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> secondFunction) {
  // std::cout << "GenConstant composing" << std::endl;
  if (dynamic_cast<IdentityEdgeFunction *>(secondFunction.get()) ||
      dynamic_cast<AllBottom<IDEGeneralizedLCA::v_t> *>(secondFunction.get())) {

    return shared_from_this();
  }
  if (dynamic_cast<GenConstant *>(secondFunction.get()))
    return secondFunction;
  // return std::make_shared<EdgeFunctionComposer>(shared_from_this(),
  //                                              secondFunction, maxSize);
  return std::make_shared<GenConstant>(secondFunction->computeTarget(val),
                                       maxSize);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> GenConstant::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> other) {
  if (auto otherConst = dynamic_cast<GenConstant *>(other.get())) {
    switch (compare(val, otherConst->val)) {
    case Ordering::Equal:
    case Ordering::Greater:
      return shared_from_this();
    case Ordering::Less:
      return other;
    default:
      return std::make_shared<GenConstant>(join(val, otherConst->val, maxSize),
                                           maxSize);
    }
  }
  if (AllBot::isBot(other))
    return AllBot::getInstance();
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), other, maxSize);
}

bool GenConstant::equal_to(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> other) const {
  if (auto otherConst = dynamic_cast<GenConstant *>(other.get())) {
    return val == otherConst->val && maxSize == otherConst->maxSize;
  }
  return false;
}
void GenConstant::print(std::ostream &OS, bool isForDebug) const {
  OS << "GenConstantEdgeFn(" << val << ")";
}

} // namespace psr
