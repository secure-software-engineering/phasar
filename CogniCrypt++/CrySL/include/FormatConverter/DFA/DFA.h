#pragma once
#include <unordered_set>

namespace CCPP {
namespace DFA {
/// \brief This class represents an interface for a *deterministic* finite state
/// machine
class DFA {
public:
  virtual ~DFA() = default;
  using State = int;
  using Input = int;
  /// \brief Retrieves the following state to src when the input is inp.
  ///
  /// This is the core of the DFA and defines the delta-function of the state
  /// machine
  virtual State getNextState(State src, Input inp) const = 0;
  /// \brief True, iff the given state stat is the final accepting state
  virtual bool isAcceptingState(State stat) const = 0;
  /// \brief True, iff the given state stat is the initial starting state
  virtual bool isInitialState(State stat) const = 0;
  /// \brief Retrieves the initial starting state
  virtual State getInitialState() const = 0;
  /// \brief Retrieves the final accepting state
  virtual const std::unordered_set<State> &getAcceptingState() const = 0;
  /// \brief Retrieves the final error state
  ///
  /// This is defaulted to -1
  virtual State getErrorState() const { return -1; }
};
} // namespace DFA
} // namespace CCPP