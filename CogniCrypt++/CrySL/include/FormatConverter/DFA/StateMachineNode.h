#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CCPP {
namespace DFA {
class StateMachine;
/// \brief A state-node of a (finite) state machine
///
/// Each state-node has an ID (retrieved by getState()), which is unique wrt.
/// the state machine which owns this node
class StateMachineNode {
public:
  using State = int;
  class Marker {
    StateMachineNode *nod;

  public:
    Marker(StateMachineNode &nod);
    ~Marker();
  };

private:
  State state;

  std::unordered_map<std::string, std::unordered_set<StateMachineNode *>> next;
  bool initial, accepting;
  bool isDet = true;
  bool marked = false;

public:
  /// \brief Initializes a new state-node. Do not call it yourself! Instead use
  /// StateMachine::addState()
  StateMachineNode(State state, bool initial = false, bool accepting = false);
  StateMachineNode(const StateMachineNode &) = delete;
  StateMachineNode(StateMachineNode &&) = delete;

  /// \brief Makes this an accepting state
  void makeAccepting();

  /// \brief The unique state-ID
  State getState() const;
  /// \brief Adds a new transition from this state to dest for the consumed
  /// input evt
  ///
  /// \returns True, iff this transition was successfully added
  bool addTransition(const std::string &evt, StateMachineNode &dest);
  void addTransitions(const std::string &evt,
                      const std::unordered_set<StateMachineNode*> &dest);
  bool removeAllTransitions(const std::string &evt);
  /// \brief True, iff this is the starting state
  bool isInitial() const;
  /// \brief True, iff this is the final accepting state
  bool isAccepting() const;
  /// \brief True, iff for each transition-label there is at most one successor
  /// state
  bool isDeterministic() const;
  bool isMarked() const;
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
  friend class StateMachine;
};

} // namespace DFA
} // namespace CCPP