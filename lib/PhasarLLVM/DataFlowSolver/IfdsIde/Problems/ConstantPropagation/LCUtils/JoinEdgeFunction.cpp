#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/JoinEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/AllBot.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/GenConstant.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/IdentityEdgeFunction.h"

namespace psr::LCUtils {
JoinEdgeFunction::JoinEdgeFunction(
    const std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
        &frst,
    const std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
        &scnd,
    size_t maxSize)
    : frst(frst), scnd(scnd), maxSize(maxSize) {

  // check for endless recursion
  // This only used for debug purposes. So you can safely remove it, but you
  // also may use it, if there are termination problems

  std::unordered_set<psr::EdgeFunction<IDELinearConstantPropagation::v_t> *>
      seen;
  std::vector<psr::EdgeFunction<IDELinearConstantPropagation::v_t> *> q = {
      frst.get(), scnd.get()};
  unsigned ctr = 0;
  while (!q.empty()) {
    auto top = q.back();
    ctr++;
    q.pop_back();
    auto ins = seen.insert(top);
    if (!ins.second &&
        !dynamic_cast<psr::AllBottom<IDELinearConstantPropagation::v_t> *>(
            top)) {
      std::cerr << "WARNING: cyclic dependency! @" << ctr << "#";
      top->print(std::cerr);
      std::cerr << std::endl;
      this->frst = this->scnd = AllBot::getInstance();
      break;
    } else {
      if (auto topJoin = dynamic_cast<JoinEdgeFunction *>(top)) {
        q.push_back(topJoin->frst.get());
        q.push_back(topJoin->scnd.get());
      } else if (auto topComp = dynamic_cast<EdgeFunctionComposer *>(top)) {
        q.push_back(topComp->getFirst().get());
        q.push_back(topComp->getSecond().get());
      }
    }
  }
}
IDELinearConstantPropagation::v_t
JoinEdgeFunction::computeTarget(IDELinearConstantPropagation::v_t source) {

  return join(frst->computeTarget(source), scnd->computeTarget(source),
              maxSize);
}

std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
JoinEdgeFunction::composeWith(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
        secondFunction) {
  // std::cout << "JoinFn composing" << std::endl;
  // TODO be more precise here
  if (dynamic_cast<GenConstant *>(secondFunction.get())) {
    return secondFunction;
  }
  if (dynamic_cast<IdentityEdgeFunction *>(secondFunction.get())) {
    return shared_from_this();
  }
  return std::make_shared<EdgeFunctionComposer>(shared_from_this(),
                                                secondFunction, maxSize);
}

std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
JoinEdgeFunction::joinWith(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
        otherFunction) {
  if (otherFunction.get() == this)
    return shared_from_this();
  if (AllBot::isBot(otherFunction)) {
    return AllBot::getInstance();
  }

  return std::make_shared<JoinEdgeFunction>(shared_from_this(), otherFunction,
                                            maxSize);
}

bool JoinEdgeFunction::equal_to(
    std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> other)
    const {

  if (this == other.get())
    return true;
  if (auto otherJoin = dynamic_cast<JoinEdgeFunction *>(other.get())) {
    return (frst->equal_to(otherJoin->frst) &&
            scnd->equal_to(otherJoin->scnd)) // join is commutative...
           ||
           (frst->equal_to(otherJoin->scnd) && scnd->equal_to(otherJoin->frst));
  }
  return false;
}
void JoinEdgeFunction::print(std::ostream &OS, bool isForDebug) const {
  OS << "JoinEdgeFn[";
  frst->print(OS);
  OS << ", ";
  scnd->print(OS);
  OS << "]";
}
const std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> &
JoinEdgeFunction::getFirst() const {
  return frst;
}
const std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> &
JoinEdgeFunction::getSecond() const {
  return scnd;
}

} // namespace psr::LCUtils