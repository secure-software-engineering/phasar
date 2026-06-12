#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/CFG.h"
#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/ControlFlow/ICFG.h"
#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/SCCGeneric.h"

namespace psr {

// TODO: Use SCCGeneric algorithms here!

// Note: Use forward edges (i.e., cs->callee), such that the SCC-order reflects
// the bottom-up iteration order.
template <typename N, typename F, CFGOf<N, F> C>
  requires InstructionClassifier<C>
[[nodiscard]] SCCHolder<FunctionId>
computeCGSCCs(const psr::CallGraph<N, F> &CG, const C &CF,
              const FunctionCompressor<F> &Functions) {

  SCCHolder<FunctionId> Ret{};

  auto NumFuns = Functions.size();
  if (!NumFuns) {
    return Ret;
  }

  Ret.SCCOfNode.resize(NumFuns);

  llvm::SmallVector<uint32_t, 128> Disc(NumFuns, UINT32_MAX);
  llvm::SmallVector<uint32_t, 128> Low(NumFuns, UINT32_MAX);
  BitSet<FunctionId> OnStack(NumFuns);
  BitSet<FunctionId> Seen(NumFuns);

  llvm::SmallVector<FunctionId> Stack;
  uint32_t Time = 0;

  constexpr auto SetMin = [](uint32_t &InOut, uint32_t Other) {
    if (Other < InOut) {
      InOut = Other;
    }
  };

  const auto Dfs = [&](auto &&Dfs, FunctionId CurrNode) -> void {
    auto CurrTime = Time++;
    Disc[size_t(CurrNode)] = CurrTime;
    Low[size_t(CurrNode)] = CurrTime;
    Stack.push_back(CurrNode);
    OnStack.insert(CurrNode);

    const auto &CurrFun = Functions[CurrNode];
    for (const auto &Inst : CF.getAllInstructionsOf(CurrFun)) {
      if (!CF.isCallSite(Inst)) {
        continue;
      }

      for (const auto &Succ : CG.getCalleesOfCallAt(Inst)) {
        auto SuccNode = Functions.get(Succ);
        if (Disc[size_t(SuccNode)] == UINT32_MAX) {
          // Tree-edge: Not seen yet --> recurse

          Dfs(Dfs, SuccNode);
          SetMin(Low[size_t(CurrNode)], Low[size_t(SuccNode)]);
        } else if (OnStack.contains(SuccNode)) {
          // Back-edge --> circle!

          SetMin(Low[size_t(CurrNode)], Disc[size_t(SuccNode)]);
        }
      }
    }

    if (Low[size_t(CurrNode)] == Disc[size_t(CurrNode)]) {
      // Found SCC

      auto SCCIdx = SCCId<FunctionId>(Ret.NodesInSCC.size());
      auto &FunsInSCC = Ret.NodesInSCC.emplace_back();

      assert(!Stack.empty());

      while (Stack.back() != CurrNode) {
        auto Fun = Stack.pop_back_val();
        Ret.SCCOfNode[Fun] = SCCIdx;
        OnStack.erase(Fun);
        Seen.insert(Fun);
        FunsInSCC.push_back(Fun);
      }

      auto Fun = Stack.pop_back_val();
      Ret.SCCOfNode[Fun] = SCCIdx;
      OnStack.erase(Fun);
      Seen.insert(Fun);
      FunsInSCC.push_back(Fun);
    }
  };

  for (auto FunId : iota<FunctionId>(NumFuns)) {
    if (!Seen.contains(FunId)) {
      Dfs(Dfs, FunId);
    }
  }

  return Ret;
}

template <ICFG I>
  requires InstructionClassifier<I>
[[nodiscard]] SCCHolder<FunctionId>
computeCGSCCs(const I &ICF,
              const FunctionCompressor<typename I::f_t> &Functions) {
  return computeCGSCCs(ICF.getCallGraph(), ICF, Functions);
}

template <typename N, typename F>
[[nodiscard]] SCCDependencyGraph<FunctionId>
computeCGSCCCallers(const psr::CallGraph<N, F> &CG, const CFGOf<N, F> auto &CF,
                    const FunctionCompressor<F> &Functions,
                    const SCCHolder<FunctionId> &SCCs) {
  SCCDependencyGraph<FunctionId> Ret;
  Ret.ChildrenOfSCC.resize(SCCs.size());

  BitSet<SCCId<FunctionId>> Leaves(SCCs.size(), true);

  for (auto [FunId, Fun] : Functions.enumerate()) {
    auto SCC = SCCs.SCCOfNode[FunId];

    for (const auto &CS : CG.getCallersOf(Fun)) {
      const auto &CSFun = CF.getFunctionOf(CS);
      if (auto CSFunId = Functions.getOrNull(CSFun)) {
        auto CSFunSCC = SCCs.SCCOfNode[*CSFunId];
        if (CSFunSCC != SCC) {
          Ret.ChildrenOfSCC[SCC].insert(CSFunSCC);
          Leaves.erase(CSFunSCC);
        }
      }
    }
  }

  Ret.SCCRoots.reserve(Leaves.size());
  Leaves.foreach (
      [&](auto Leaf) { Ret.SCCRoots.push_back(SCCId<FunctionId>(Leaf)); });

  return Ret;
}

template <ICFG I>
[[nodiscard]] SCCDependencyGraph<FunctionId>
computeCGSCCCallers(const I &ICF,
                    const FunctionCompressor<typename I::f_t> &Functions,
                    const SCCHolder<FunctionId> &SCCs) {
  return computeCGSCCCallers(ICF.getCallGraph(), ICF, Functions, SCCs);
}

} // namespace psr
