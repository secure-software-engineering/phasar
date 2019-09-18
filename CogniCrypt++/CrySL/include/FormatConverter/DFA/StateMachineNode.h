#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CCPP {
namespace DFA {
/// \brief A state-node of a (finite) state machine
///
/// Each state-node has an ID (retrieved by getState()), which is unique wrt.
/// the state machine which owns this node
class StateMachineNode {
public:
  using State = int;

private:
  State state;

  std::unordered_map<std::string, std::unordered_set<StateMachineNode *>> next;
  bool initial, accepting;
  bool isDet = true;

public:
  /// \brief Initializes a new state-node. Do not call it yourself! Instead use
  /// StateMachine::addState()
  StateMachineNode(State state, bool initial = false, bool accepting = false);
  StateMachineNode(const StateMachineNode &) = delete;
  StateMachineNode(StateMachineNode &&) = delete;
  /// \brief The unique state-ID
  State getState() const;
  /// \brief Adds a new transition from this state to dest for the consumed
  /// input evt
  ///
  /// \returns True, iff this transition was successfully added
  bool addTransition(const std::string &evt, StateMachineNode &dest);
  /// \brief True, iff this is the starting state
  bool isInitial() const;
  /// \brief True, iff this is the final accepting state
  bool isAccepting() const;
  bool isDeterministic() const;
  /// \brief Performs a state-transition with input label
  ///
  /// \returns All possible successor states under the input label. This vector
  /// may be empty
  std::vector<std::reference_wrapper<StateMachineNode>>
  getNextState(const std::string &label) const;

  const std::unordered_map<std::string, std::unordered_set<StateMachineNode *>>
      &getMap() const;
  const std::unordered_map<std::string, StateMachineNode *>
  getDeterministicMap() const;
};

} // namespace DFA
} // namespace CCPP