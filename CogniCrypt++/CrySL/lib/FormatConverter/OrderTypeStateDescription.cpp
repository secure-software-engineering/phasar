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
psr::TypeStateDescription::State OrderTypeStateDescription::getNextState(std::string Tok, State S) const {
  auto it = eventStates.find(Tok);
  if (it == eventStates.end())
    return error();
  // TODO set the objects' values (or better do this in the snslysis description
  // itself? => would require us to create a subclass of
  // psr::IDETypeStateAnalysis)
  return dfa->getNextState(S, it->second);
}
std::string OrderTypeStateDescription::getTypeNameOfInterest() const {
  return typeName;
}
std::string OrderTypeStateDescription::stateToString(State S)const {
  if (S == -1)
    return "<ERROR-STATE>";
  for (auto &kvp : eventStates) {
    if (kvp.second == S) {
      return kvp.first;
    }
  }
  return "<NO-STATE>" ;
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
psr::TypeStateDescription::State OrderTypeStateDescription::error() const { return -1; }
psr::TypeStateDescription::State OrderTypeStateDescription::accepting() const {
  return dfa->getAcceptingState();
}
// TODO implement rest
} // namespace CCPP
