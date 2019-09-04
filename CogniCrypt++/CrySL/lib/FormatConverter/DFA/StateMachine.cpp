#include <DFA/DFA.h>
#include <DFA/StateMachine.h>


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
  // TODO implement
  return nullptr;
}
} // namespace DFA
} // namespace CCPP
