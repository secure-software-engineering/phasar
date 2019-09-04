#pragma once
#include <Event.h>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace CCPP {
namespace DFA {
/// \brief A state-node of a (finite) state machine
///
/// Each state-node has an ID (retrieved by getState()), which is unique wrt.
/// the state machine which owns this node
class StateMachineNode {
  using State = int;
  State state;

  std::unordered_multimap<std::string, std::reference_wrapper<StateMachineNode>>
      next;
  bool initial, accepting;

public:
  /// \brief Initializes a new state-node. Do not call it yourself! Instead use
  /// StateMachine::addState()
  StateMachineNode(State state, bool initial = false, bool accepting = false);
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
  /// \brief Performs a state-transition with input label
  ///
  /// \returns All possible successor states under the input label. This set may
  /// be empty
  std::unordered_set<std::reference_wrapper<StateMachineNode>>
  getNextState(const std::string &label) const;
};

} // namespace DFA
} // namespace CCPP