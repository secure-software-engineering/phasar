#include <FormatConverter/DFA/DFA.h>
#include <FormatConverter/DFA/DFStateMachine.h>
#include <FormatConverter/DFA/StateMachine.h>
#include <FormatConverter/DFA/StateMachineNode.h>
#include <set>

namespace std {
template <typename T> struct hash<set<T *>> {
  size_t operator(const unordered_set<T *> &s) {
    size_t sum = 17;
    for (auto e : s)
      sum += 31 * (size_t)s + 97;
    return std::hash(sum);
  }
};
} // namespace std

namespace CCPP {
namespace DFA {
using namespace std;

StateMachine::StateMachine() {
  auto initial = make_unique<StateMachineNode>(0, true);
  auto accepting = make_unique<StateMachineNode>(1, false, true);
  states.push_back(move(initial));
  states.push_back(move(accepting));
}

StateMachineNode &StateMachine::addState() {
  states.push_back(make_unique<StateMachineNode>((int)states.size()));
  return *states.back();
}
StateMachineNode &StateMachine::addState(bool accepting) {
  states.push_back(
      make_unique<StateMachineNode>((int)states.size(), false, accepting));
  return *states.back();
}
void StateMachine::makeInitialStateAccepting() { states[0]->initial = true; }
StateMachineNode &StateMachine::getInitialState() const { return *states[0]; }
StateMachineNode &StateMachine::getAcceptingState() const { return *states[1]; }
unique_ptr<DFA> StateMachine::convertToDFA() const {

  DFA::State initial = 0;
  unordered_set<DFA::State> accepting;
  vector<vector<DFA::State>> delta; // DFA::State is int
  unordered_map<set<StateMachineNode *>, StateMachineNode *> dsmMap;
  StateMachine stateMachine;
  for (auto &ref : states) {
    set<StateMachineNode *> smNodeSet;
    StateMachineNode *smNode;
    for (auto var2 : ref->getNextState("")) {
      smNodeSet.insert(&var2.get());
      if (ref->isInitial()) {
        smNode = &stateMachine.getInitialState();
      } else if (ref->isAccepting()) {
        smNode = &stateMachine.addState(true);
      } else {
        smNode = &stateMachine.addState();
      }
    }
    dsmMap.insert({smNodeSet, smNode});

    for (auto var :
         ref->getMap()) { // map for each state machine node key= transition
                          // value set= possible destinations nodes
      for (auto setVar : var.second) {
        smNode->addTransition(var.first, *setVar);
      }
    }
  }

  //TODO : add more nodes in dsmMap after updating the smNode and smNodeSet

  // nested pushback for delta
  // ith index means transition i for certain state
  return make_unique<DFStateMachine>(initial, accepting, move(delta));
}
bool StateMachine::isDeterministic() const {
  for (auto &stat : states) {
    if (!stat->isDeterministic())
      return false;
  }
  return true;
}
std::vector<std::vector<DFA::State>>
StateMachine::createAdjacenceMatrix() const {
  // TODO implement
}
} // namespace DFA
  // namespace DFA
} // namespace CCPP
