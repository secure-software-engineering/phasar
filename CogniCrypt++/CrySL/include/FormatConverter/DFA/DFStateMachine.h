#include "DFA.h"
#include <array>
#include <memory>

namespace CCPP {
namespace DFA {
template <size_t NumStates, size_t NumInputs>
class DFStateMachine : public DFA {
  std::array<std::array<State, NumInputs>, NumStates> delta;
  State initial, accepting;

public:
  DFStateMachine(State initial, State accepting,
                 std::array<std::array<State, NumInputs>, NumStates> &&delta)
      : initial(initial), accepting(accepting), delta(std::move(delta)) {}
  State getNextState(State src, Input inp) const override {
    return delta.at(src).at(inp);
  }
  bool isAcceptingState(State stat) const override { return stat == accepting; }
  bool isInitialState(State stat) const override { return stat == initial; }
  State getInitialState() const override { return initial; }
  State getAcceptingState() const override { return accepting; }
};
} // namespace DFA
} // namespace CCPP