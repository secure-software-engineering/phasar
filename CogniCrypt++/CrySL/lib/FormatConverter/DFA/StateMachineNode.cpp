#include <DFA/StateMachineNode.h>

namespace CCPP {
namespace DFA {
using namespace std;

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
                                     StateMachineNode &dest) {
  auto ret = next[evt].insert(&dest);
  return ret.second;
}

} // namespace DFA
} // namespace CCPP