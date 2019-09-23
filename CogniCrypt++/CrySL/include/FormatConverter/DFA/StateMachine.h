#pragma once
#include "StateMachineNode.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CCPP {
namespace DFA {
class DFA;
/// \brief A nondeterministic finite state-machine with string-labels
///
/// Can be converted to a DFA
class StateMachine {
  std::vector<std::unique_ptr<StateMachineNode>> states;

  std::vector<std::vector<int>>
  createAdjacenceMatrix(std::unordered_map<std::string, int> &evtTrn,
                        std::unordered_set<int> &acceptingStates) const;
  void eliminateEpsilonTransitions();

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
  /// \param eventTransitions A map from the string-representation to the
  /// integer-id of all used transitions. This map  will be filled by this
  /// method
  ///
  /// \returns An owning pointer to the newly created DFA
  std::unique_ptr<DFA>
  convertToDFA(std::unordered_map<std::string, int> &eventTransitions);

  bool isDeterministic() const;
};
} // namespace DFA
} // namespace CCPP