#include <FormatConverter/DFA/DFA.h>
#include <FormatConverter/DFA/DFStateMachine.h>
#include <FormatConverter/DFA/StateMachine.h>
#include <FormatConverter/DFA/StateMachineNode.h>

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
StateMachineNode &StateMachine::getInitialState() const { return *states[0]; }
StateMachineNode &StateMachine::getAcceptingState() const { return *states[1]; }
unique_ptr<DFA> StateMachine::convertToDFA() const {

  DFA::State initial = 0;
  unordered_set<DFA::State> accepting;
  vector<vector<DFA::State>> delta; // DFA::State is int
  unordered_map<string, unordered_set<DFA::State>> stateTransition;




/*
  for (auto &ref : states) {
    for (auto varMap :
         ref->getMap()) { // map for each state machine node key= transition
                          // value set= possible destinations nodes
      if (int(varMap.first == i1)) { // casting map key to int
           delta.at(i1).push_back();
          }
        }
        auto it = delta.begin();
      

    // nested pushback for delta
    // ith index means transition i for certain state
  }
*/
  return make_unique<DFStateMachine>(initial, accepting, move(delta));
}
}
 // namespace DFA
} // namespace CCPP
