#pragma once

namespace CCPP {
namespace DFA {
/// \brief This class represents an interface for a *deterministic* finite state
/// machine
class DFA {
public:
  virtual ~DFA() = default;
  using State = int;
  using Input = int;
  virtual State getNextState(State src, Input inp) const = 0;
  virtual bool isAcceptingState(State stat) const = 0;
  virtual bool isInitialState(State stat) const = 0;
  virtual State getInitialState() const = 0;
  virtual State getAcceptingState() const = 0;
  virtual State getErrorState() const { return -1; }
};
} // namespace DFA
} // namespace CCPP