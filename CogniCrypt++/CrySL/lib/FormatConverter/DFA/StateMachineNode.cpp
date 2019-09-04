#include <DFA/StateMachineNode.h>

namespace CCPP {
namespace DFA {

StateMachineNode::StateMachineNode(State state, bool initial, bool accepting)
    : state(state), initial(initial), accepting(accepting) {}

StateMachineNode::State StateMachineNode::getState() const { return state; }
bool StateMachineNode::isInitial() const { return initial; }
bool StateMachineNode::isAccepting() const { return accepting; }
vector<reference_wrapper<StateMachineNode>>
StateMachineNode::getNextState(const string &label) const {

  vector<reference_wrapper<StateMachineNode>> ret;
  auto it = next.find(label);
  if (it != next.end()) {
    for (auto nod : it->second) {
      ret.emplace_back(*nod);
    }
  }
  return ret;
}
bool StateMachineNode::addTransition(const string &evt,
                                     STateMachineNode &dest) {
  // TODO implement
}

} // namespace DFA
} // namespace CCPP