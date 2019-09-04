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
  StateMachine();
  /// \brief Creates a new StateMachineNode which is owned by this NFA
  ///
  /// \returns A reference to the newly created StateMachineNode
  StateMachineNode &addState();
  StateMachineNode &getInitialState() const;
  StateMachineNode &getAcceptingState() const;
  /// \brief Creates a new DFA from this NFA which decides exactly the same
  /// language as this NFA.
  ///
  /// \returns An owning pointer to the newly created DFA
  std::unique_ptr<DFA> convertToDFA() const;
};
} // namespace DFA
} // namespace CCPP