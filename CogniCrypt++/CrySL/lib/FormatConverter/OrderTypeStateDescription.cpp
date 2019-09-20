#include <FormatConverter/OrderTypeStateDescription.h>

namespace CCPP {
OrderTypeStateDescription::OrderTypeStateDescription(
    const std::string &typeName, std::unique_ptr<DFA::DFA> &&dfs,
    std::unordered_map<std::string, int> &&eventTransitions)
    : dfa(std::move(dfs)), eventTransitions(std::move(eventTransitions)),
      typeName(typeName) {
  // TODO: mangle typeName
}

bool OrderTypeStateDescription::isAPIFunction(const std::string &F) const {
  return eventTransitions.count(F);
}
psr::TypeStateDescription::State
OrderTypeStateDescription::getNextState(std::string Tok, State S) const {
  if (S == error())
    return S;
  auto it = eventTransitions.find(Tok);
  if (it == eventTransitions.end())
    return error();
  return dfa->getNextState(S, it->second);
}
std::string OrderTypeStateDescription::getTypeNameOfInterest() const {
  return typeName;
}
std::string OrderTypeStateDescription::stateToString(State S) const {
  if (S == -1)
    return "<ERROR-STATE>";
  for (auto &kvp : eventTransitions) {
    if (kvp.second == S) {
      return kvp.first;
    }
  }
  return "<NO-STATE>";
}
psr::TypeStateDescription::State OrderTypeStateDescription::bottom() const {
  // TODO implement;
  return -1;
}
psr::TypeStateDescription::State OrderTypeStateDescription::top() const {
  // TODO implement;
  return -1;
}
psr::TypeStateDescription::State OrderTypeStateDescription::uninit() const {
  return dfa->getInitialState();
}
psr::TypeStateDescription::State OrderTypeStateDescription::start() const {
  // TODO implement
  return 0;
}
psr::TypeStateDescription::State OrderTypeStateDescription::error() const {
  return -1;
}
psr::TypeStateDescription::State OrderTypeStateDescription::accepting() const {
  return dfa->getAcceptingState();
}
// TODO implement rest
} // namespace CCPP
