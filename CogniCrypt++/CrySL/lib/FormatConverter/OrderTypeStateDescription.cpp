#include <OrderTypeStateDescription.h>

namespace CCPP {
OrderTypeStateDescription::OrderTypeStateDescription(
    const std::string &typeName, std::unique_ptr<DFA::DFA> &&dfs,
    std::unordered_map<std::string, int> &&eventStates)
    : dfa(std::move(dfa)), eventStates(std::move(eventStates)),
      typeName(typeName) {}

bool OrderTypeStateDescription::isAPIFunction(const std::string &F) const {
  return eventStates.count(F);
}
std::string OrderTypeStateDescription::getTypeNameOfInterest() const {
  return typeName;
}
std::string stateToString(State S) {
  if (S == -1)
    return "<ERROR-STATE>";
  for (auto &kvp : eventStates) {
    if (kvp.second == S) {
      return kvp.first;
    }
  }
  return "<NO-STATE>";
}
State OrderTypeStateDescription::bottom() const {
  // TODO implement;
  return -1;
}
State OrderTypeStateDescription::top() const {
  // TODO implement;
  return -1;
}
State OrderTypeStateDescription::uninit() const {
  return dfa->getInitialState();
}
State OrderTypeStateDescription::start() const {
  // TODO implement
  return 0;
}
State OrderTypeStateDescription::error() const { return -1; }
State OrderTypeStateDescription::accepting() const {
  return dfa->getAcceptingState();
}
// TODO implement rest
} // namespace CCPP
