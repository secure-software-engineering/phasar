#include <FormatConverter/DFA/DFA.h>
#include <FormatConverter/DFA/DFStateMachine.h>
#include <FormatConverter/DFA/StateMachine.h>
#include <FormatConverter/DFA/StateMachineNode.h>
#include <functional>
#include <list>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>

namespace std {
/*template <typename T> struct hash<set<T *>> {
  typedef T argument_type;
  typedef size_t result_type;
  size_t operator()(const set<T *> &s) const {
    size_t sum = 17;
    for (auto e : s)
      sum += 31 * (size_t)s + 97;
    return std::hash<size_t>()(sum);
  }
};
template <typename T> struct hash<reference_wrapper<T>> {
  typedef reference_wrapper<T> argument_type;
  typedef size_t result_type;
  size_t operator()(const reference_wrapper<T> &r) const {
    return std::hash()(r.get());
  }
};*/
template <typename T> struct hash<reference_wrapper<set<T *>>> {
  typedef T argument_type;
  typedef size_t result_type;
  size_t operator()(const reference_wrapper<set<T *>> &s) const {
    size_t sum = 17;
    for (auto e : s.get())
      sum += 31 * (size_t)e + 97;
    return std::hash<size_t>()(sum);
  }
};
template <typename T> struct equal_to<reference_wrapper<T>> {
  bool operator()(const reference_wrapper<T> &r1,
                  const reference_wrapper<T> &r2) const {
    return r1.get() == r2.get();
  }
};
} // namespace std

namespace CCPP {
namespace DFA {
using namespace std;

StateMachine::StateMachine() {
  auto initial = make_unique<StateMachineNode>(0, true);
  auto accepting = make_unique<StateMachineNode>(1, false, true);
  states.push_back(move(initial));
  states.push_back(move(accepting));
}

StateMachineNode &StateMachine::addState() {
  states.push_back(make_unique<StateMachineNode>((int)states.size()));
  return *states.back();
}
StateMachineNode &StateMachine::addState(bool accepting) {
  states.push_back(
      make_unique<StateMachineNode>((int)states.size(), false, accepting));
  return *states.back();
}
void StateMachine::makeInitialStateAccepting() { states[0]->initial = true; }
StateMachineNode &StateMachine::getInitialState() const { return *states[0]; }
StateMachineNode &StateMachine::getAcceptingState() const { return *states[1]; }
unique_ptr<DFA>
StateMachine::convertToDFA(unordered_map<string, int> &eventTransitions) {
  this->eliminateEpsilonTransitions();

  StateMachine dfsm;
  // use list instead of vector for pointer stability
  std::list<std::set<StateMachineNode *>> setStateOwner = {
      {&this->getInitialState()}};
  std::unordered_map<std::reference_wrapper<std::set<StateMachineNode *>>,
                     StateMachineNode *>
      setStates = {{std::ref(setStateOwner.front()), &dfsm.getInitialState()}};

  auto getOrCreateSetState = [&](std::set<StateMachineNode *> &&q)
      -> std::tuple<StateMachineNode *,
                    std::reference_wrapper<std::set<StateMachineNode *>>> {
    auto it = setStates.find(q);
    if (it != setStates.end())
      return {it->second, q};

    bool acc = false;
    for (auto stat : q) {
      if (stat->isAccepting())
        acc = true;
    }
    setStateOwner.push_back(std::move(q));
    return {setStates[setStateOwner.back()] = &dfsm.addState(acc),
            setStateOwner.back()};
  };

  std::unordered_set<std::reference_wrapper<std::set<StateMachineNode *>>>
      finished;
  std::queue<std::reference_wrapper<std::set<StateMachineNode *>>>
      processingStates;
  processingStates.push(setStateOwner.front());
  while (!processingStates.empty()) {
    // merge indistinguishable states (powerset-construction)
    auto &currStat = processingStates.front().get();
    auto detState = setStates[currStat];
    processingStates.pop();
    if (finished.count(currStat))
      continue;

    std::unordered_map<std::string, std::set<StateMachineNode *>> combinedTrn;
    for (auto q : currStat) {
      for (auto &kvp : q->getMap()) {
        auto &dest = combinedTrn[kvp.first];
        dest.insert(kvp.second.begin(), kvp.second.end());
      }
    }

    for (auto &kvp : combinedTrn) {
      auto succ = getOrCreateSetState(std::move(kvp.second));
      auto succStat = std::get<0>(succ);
      detState->addTransition(kvp.first, *succStat);
      processingStates.push(std::get<1>(succ));
    }

    finished.emplace(currStat);
  }
  // TODO: Maybe minimize the dfsm after powerset-construction

  // create delta-matrix and instantiate DFStateMachine
  std::unordered_set<int> acceptingStates;
  auto delta = dfsm.createAdjacenceMatrix(eventTransitions, acceptingStates);

  return std::make_unique<DFStateMachine>(0, std::move(acceptingStates),
                                          std::move(delta));
} // namespace DFA
bool StateMachine::isDeterministic() const {
  for (auto &stat : states) {
    if (!stat->isDeterministic())
      return false;
  }
  return true;
}

/// \brief traverses the induces sub-NFA, which only uses epsilon-transitions
/// and creates the transitive closure of it (not reflexive)
std::tuple<std::reference_wrapper<std::unordered_set<StateMachineNode *>>, bool>
traverseRec(StateMachineNode *q,
            std::unordered_map<StateMachineNode *,
                               std::unordered_set<StateMachineNode *>> &closure,
            std::unordered_set<StateMachineNode *> &closure_finished) {

  if (closure_finished.count(q)) {
    // prevent recomputation
    return {std::ref(closure[q]), true};
  } else if (q->isMarked()) {
    // prevent endless loops
    return {std::ref(closure[q]), false};
  }

  StateMachineNode::Marker m(*q);
  auto &clos = closure[q];
  bool noLoops = true;

  for (auto &succ : q->getNextState("")) {
    clos.insert(&succ.get());
    auto succ_res = traverseRec(&succ.get(), closure, closure_finished);
    if (succ.get().isAccepting())
      q->makeAccepting();
    auto &succ_clos = std::get<0>(succ_res).get();
    noLoops &= std::get<1>(succ_res);
    clos.insert(succ_clos.begin(), succ_clos.end());
  }
  if (noLoops)
    // else, we might only have a subset of the closure
    closure_finished.insert(q);

  return {std::ref(clos), noLoops};
}

void StateMachine::eliminateEpsilonTransitions() {
  // 1. for each state q: create the epsilon-closure C[q] (we don't include q in
  // C[q] here)
  // 2. for each state q: add each non-epsilon transition from C[q] to q
  // 3. simply remove all epsilon-transitions

  // 1.)

  std::unordered_map<StateMachineNode *, std::unordered_set<StateMachineNode *>>
      closure;
  std::unordered_set<StateMachineNode *> closure_finished;
  bool hasEpsilons = false;

  const auto traverse = [&](StateMachineNode *q) {
    return traverseRec(q, closure, closure_finished);
  };
  for (auto &stat : states) {
    if (!closure_finished.count(stat.get())) {
      traverse(stat.get());
      closure_finished.insert(stat.get());
      hasEpsilons |= !closure[stat.get()].empty();
    }
  }
  if (!hasEpsilons)
    return;
  // 2.), 3.)

  for (auto &stat_clos : closure) {
    stat_clos.first->removeAllTransitions("");
    for (auto stat : stat_clos.second) {
      for (auto trn : stat->getMap()) {
        if (!trn.first.empty()) {
          stat_clos.first->addTransitions(trn.first, trn.second);
        }
      }
    }
  }
}

} // namespace DFA
} // namespace CCPP

// create the delta-matrix
#include "AdjacenceMatrixCreator.h"