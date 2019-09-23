#include <FormatConverter/OrderConverter.h>
#include <FormatConverter/OrderTypeStateDescription.h>
#include <unordered_map>
#include <unordered_set>

namespace CCPP {
OrderConverter::OrderConverter(const std::string &specName,
                               CrySLParser::OrderContext *order,
                               CrySLParser::EventsContext *evt)
    : specName(specName), order(order), evt(evt) {}
std::unique_ptr<psr::TypeStateDescription>
OrderConverter::convert(const CrySLTypechecker &ctc) {
  //  create a DFA::StateMachine;
  std::unique_ptr<DFA::DFA> DFA;
  std::unordered_map<std::string, int> eventTransitions;
  {
    auto NFA = createFromContext(order, evt);
    // create a DFA::DFStateMachine from the DFA::StateMachine
    DFA = NFA->convertToDFA(eventTransitions);
  }
  //  create a OrderTypeStateDescription and feed the DFA::DFStateMachine
  // and the "eventName to trn-id" map to it

  return std::make_unique<OrderTypeStateDescription>(
      specName, std::move(DFA), std::move(eventTransitions));
}
} // namespace CCPP