#pragma once
#include "DFA.h"
#include <memory>
#include <unordered_set>
#include <vector>

namespace CCPP {
namespace DFA {
/// \brief A concrete implementation of the DFA interface, which prioritizes
/// performance over memory usage
class DFStateMachine : public DFA {
  std::vector<std::vector<State>> delta;
  State initial;
  std::unordered_set<State> accepting;

public:
  DFStateMachine(State initial, std::unordered_set<State> &&accepting,
                 std::vector<std::vector<State>> &&delta)
      : initial(initial), accepting(std::move(accepting)),
        delta(std::move(delta)) {}
  State getNextState(State src, Input inp) const override {
    return delta.at(src).at(inp);
  }
  bool isAcceptingState(State stat) const override {
    return accepting.count(stat);
  }
  bool isInitialState(State stat) const override { return stat == initial; }
  State getInitialState() const override { return initial; }
  const std::unordered_set<State> &getAcceptingState() const override {
    return accepting;
  }
};
} // namespace DFA
} // namespace CCPP