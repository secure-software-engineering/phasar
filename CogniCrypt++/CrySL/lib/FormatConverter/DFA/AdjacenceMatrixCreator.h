#pragma once
#include <FormatConverter/DFA/DFA.h>
#include <FormatConverter/DFA/DFStateMachine.h>
#include <FormatConverter/DFA/StateMachine.h>
#include <FormatConverter/DFA/StateMachineNode.h>
#include <unordered_map>

namespace CCPP {
namespace DFA {
using namespace std;

struct AdjacenceMatrixCreator {
  unordered_map<StateMachineNode *, DFA::State> processedNodes;
  vector<vector<DFA::State>> delta;
  std::unordered_map<string, DFA::Input> &evtTrn;
  AdjacenceMatrixCreator(std::unordered_map<string, DFA::Input> &evtTrn)
      : evtTrn(evtTrn) {}

  static void reserveAndFill(vector<DFA::State> &vec, DFA::State fill,
                             size_t nwSize) {
    if (nwSize > vec.size()) {
      vec.reserve(nwSize);
      for (size_t i = vec.size(); i < nwSize; ++i) {
        vec.push_back(fill);
      }
    }
  }
  DFA::Input getTransitionID(const string &trn) {
    auto it = evtTrn.find(trn);
    if (it != evtTrn.end())
      return it->second;
    else {
      DFA::Input ret = evtTrn.size();
      evtTrn[trn] = ret;
      return ret;
    }
  }
  DFA::State getNodeID(StateMachineNode *currentNode) {
    {
      auto it = processedNodes.find(currentNode);
      if (it != processedNodes.end())
        return it->second;
    }

    processedNodes[currentNode] = delta.size();
    //  implement recursive traversal
    delta.emplace_back();
    for (auto trnSucc : currentNode->getDeterministicMap()) {
      auto trn = getTransitionID(trnSucc.first);
      auto succ = getNodeID(trnSucc.second);

      reserveAndFill(delta.back(), -1, trn + 1);
      delta.back()[trn] = succ;
    }
  }
};

std::vector<std::vector<DFA::State>> StateMachine::createAdjacenceMatrix(
    std::unordered_map<string, DFA::Input> &evtTrn) const {
  AdjacenceMatrixCreator creator(evtTrn);
  auto initialState = creator.getNodeID(states[0].get());
  // assert(initialState == 0);
  return move(creator.delta);
}
} // namespace DFA
} // namespace CCPP