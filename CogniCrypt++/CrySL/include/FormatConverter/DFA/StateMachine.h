#pragma once
#include "StateMachineNode.h"
#include <memory>
#include <vector>

namespace CCPP {
namespace DFA {
class DFA;
/// \brief A nondeterministic finite state-machine with string-labels
///
/// Can be converted to a DFA
class StateMachine {
  std::vector<std::unique_ptr<StateMachineNode>> states;

public:
  /// \brief Creates a new StateMachine with two states: the initial and the
  /// accepting state.
  StateMachine();
  /// \brief Creates a new StateMachineNode which is owned by this NFA
  ///
  /// \returns A reference to the newly created StateMachineNode
  StateMachineNode &addState();
  /// \brief Retrieves the initial starting state
  StateMachineNode &getInitialState() const;
  /// \brief Retrieves the final accepting state
  StateMachineNode &getAcceptingState() const;
  /// \brief Creates a new DFA from this NFA which decides exactly the same
  /// language as this NFA.
  ///
  /// \returns An owning pointer to the newly created DFA
  std::unique_ptr<DFA> convertToDFA() const;
};
} // namespace DFA
} // namespace CCPP