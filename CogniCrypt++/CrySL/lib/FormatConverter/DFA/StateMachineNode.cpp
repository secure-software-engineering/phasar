#include <FormatConverter/DFA/StateMachineNode.h>

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
  auto &nxtSet = next[evt];
  auto ret = nxtSet.insert(&dest);
  if (nxtSet.size() > 1)
    isDet = false;
  return ret.second;
}
bool StateMachineNode::isDeterministic() const { return isDet; }
const unordered_map<string, unordered_set<StateMachineNode *>> &
StateMachineNode::getMap() const {
  return next;
}
const unordered_map<string, StateMachineNode *>
StateMachineNode::getDeterministicMap() const {
  unordered_map<string, StateMachineNode *> ret;
  for (const auto &kvp : next) {
    if (!kvp.second.empty()) {
      ret[kvp.first] = *kvp.second.begin();
    }
  }
  return ret;
}
} // namespace DFA
} // namespace CCPP