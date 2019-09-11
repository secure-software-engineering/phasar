#include "DFA.h"
#include <vector>
#include <memory>

namespace CCPP {
namespace DFA {
/// \brief A concrete implementation of the DFA interface, which prioritizes 
/// performance over memory usage
class DFStateMachine : public DFA {
  std::vector<std::vector<State>> delta;
  State initial, accepting;

public:
  DFStateMachine(State initial, State accepting,
                 std::vector<std::vector<State>> &&delta)
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