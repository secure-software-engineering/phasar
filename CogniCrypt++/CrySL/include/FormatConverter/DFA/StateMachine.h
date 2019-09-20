#pragma once
#include "StateMachineNode.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


namespace CCPP {
namespace DFA {
class DFA;
/// \brief A nondeterministic finite state-machine with string-labels
///
/// Can be converted to a DFA
class StateMachine {
  std::vector<std::unique_ptr<StateMachineNode>> states;

  std::vector<std::vector<DFA::State>> createAdjacenceMatrix() const;

public:
  /// \brief Creates a new StateMachine with two states: the initial and the
  /// accepting state.
  StateMachine();
  /// \brief Creates a new StateMachineNode which is owned by this NFA
  ///
  /// \returns A reference to the newly created StateMachineNode
  StateMachineNode &addState();
  /// \brief Creates a new StateMachineNode which is owned by this NFA
  ///
  /// \param accepting True, iff the new state should be an accepting state
  /// \returns A reference to the newly created StateMachineNode
  StateMachineNode &addState(bool accepting);
  /// \brief Retrieves the initial starting state
  StateMachineNode &getInitialState() const;
  /// \brief Makes the initial state accepting (by default the initial state is
  /// not accepting)
  void makeInitialStateAccepting();
  /// \brief Retrieves the final accepting state
  StateMachineNode &getAcceptingState() const;
  /// \brief Creates a new DFA from this NFA which decides exactly the same
  /// language as this NFA.
  ///
  /// \returns An owning pointer to the newly created DFA
  std::unique_ptr<DFA>
  convertToDFA(std::unordered_map<std::string, int> &eventTransitions) const;

  bool isDeterministic() const;
};
} // namespace DFA
} // namespace CCPP