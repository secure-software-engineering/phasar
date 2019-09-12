#include <FormatConverter/OrderConverter.h>

namespace CCPP {
OrderConverter::OrderConverter(const std::string &specName,
                               CrySLParser::OrderContext *order,
                               CrySLParser::EventsContext *evt)
    : specName(specName) {
  // TODO store order and evt (and preprocess them)
}
std::unique_ptr<psr::TypeStateDescription> OrderConverter::convert(const CrySLTypechecker& ctc) {
  // TODO create a DFA::StateMachine;
  // TODO create a DFA::DFStateMachine from the DFA::StateMachine
  // TODO create a OrderTypeStateDescription and feed the DFA::DFStateMachine
  // and the "eventName to state-id" map to it

  //return std::make_unique<OrderTypeStateDescription>(...);
  return nullptr;
}
} // namespace CCPP