/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Martin Mory and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_MONO_IDETOMONOPROBLEM_H_
#define PHASAR_PHASARLLVM_MONO_IDETOMONOPROBLEM_H_

#include <utility>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h>

namespace psr {

template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class IDEToMonoProblem
    : public InterMonoProblem<N, std::pair<D, L>, F, T, V, I> {
private:
  using MonoFact = std::pair<D, L>;

  IDETabulationProblem<N, D, F, T, V, L, I> &IDEProblem;

  BitVectorSet<MonoFact> applyIFDSFlowFunction(const BitVectorSet<MonoFact> &In,
                                               FlowFunction<D> *FF) {
    BitVectorSet<MonoFact> Out;
    for (auto &FlowFact : In.getAsSet()) {
      auto FlowFacts = FF->computeTargets(FlowFact);
      Out.insert(FlowFacts.begin(), FlowFacts.end());
    }
    return Out;
  }

public:
  IDEToMonoProblem(IDETabulationProblem<N, D, F, T, V, I> &IDEProblem)
      : IDEProblem(IDEProblem) {}

  IDEToMonoProblem(const IDEToMonoProblem &copy) = delete;
  IDEToMonoProblem(IDEToMonoProblem &&move) = delete;
  IDEToMonoProblem &operator=(const IDEToMonoProblem &copy) = delete;
  IDEToMonoProblem &operator=(IDEToMonoProblem &&move) = delete;

  BitVectorSet<MonoFact> join(const BitVectorSet<MonoFact> &Lhs,
                              const BitVectorSet<MonoFact> &Rhs) override {
    std::set<MonoFact> Out = Lhs.getAsSet();
    for (auto &[Fact, Value] : Rhs.getAsSet()) {
        bool Found = false;
        for (auto &[OutFact, OutValue] : Out) {
            if (Fact == OutFact) {
                Found = true;
                OutValue = IDEProblem.join(Value, OutValue);
            }
        }
        if (!Found)  {
            Out.insert({Fact, Value});
        }
    }
    return BitVectorSet<MonoFact>(Out);
  }

  bool sqSubSetEqual(const BitVectorSet<MonoFact> &Lhs,
                     const BitVectorSet<MonoFact> &Rhs) override {
    return Rhs.includes(Lhs);
  }

  std::unordered_map<N, BitVectorSet<MonoFact>> initialSeeds() override {
    auto Seeds = IDEProblem.initialSeeds();
    std::unordered_map<N, BitVectorSet<MonoFact>> MonoSeeds;
    for (auto &[Inst, Facts] : Seeds) {
      MonoSeeds[Inst] = BitVectorSet<MonoFact>({Facts, IDEProblem->topElement()});
    }
    return Seeds;
  }

  BitVectorSet<MonoFact> normalFlow(N S,
                                    const BitVectorSet<MonoFact> &In) override {
    auto NFF = IDEProblem.getNormalFlowFunction(S, N{});
    BitVectorSet<MonoFact> Out;
    for (auto &[Fact, Value] : In.getAsSet()) {
      auto OutFacts = applyIFDSFlowFunction(In, NFF.get());
      for (auto &OutFact : OutFacts) {
        auto NEF = IDEProblem.getNormalEdgeFunction(S, Fact, N{}, OutFact);
        Out.insert({OutFact, NEF->computeTarget(Value)});
      }
    }
    return Out;
  }

  BitVectorSet<MonoFact> callFlow(N CallSite, F Callee,
                                  const BitVectorSet<MonoFact> &In) override {
    auto CFF = IDEProblem.getCallFlowFunction(CallSite, Callee);
    BitVectorSet<MonoFact> Out;
    for (auto &[Fact, Value] : In.getAsSet()) {
      auto OutFacts = applyIFDSFlowFunction(In, CFF.get());
      for (auto &OutFact : OutFacts) {
        auto CEF =
            IDEProblem.getCallEdgeFunction(CallSite, Fact, Callee, OutFact);
        Out.insert({OutFact, CEF->computeTarget(Value)});
      }
    }
    return Out;
  }

  BitVectorSet<MonoFact> returnFlow(N CallSite, F Callee, N ExitStmt, N RetSite,
                                    const BitVectorSet<MonoFact> &In) override {
    auto RFF =
        IDEProblem.getRetFlowFunction(CallSite, Callee, ExitStmt, RetSite);
    for (auto &[Fact, Value] : In.getAsSet()) {
      auto OutFacts = applyIFDSFlowFunction(In, CFF.get());
      for (auto &OutFact : OutFacts) {
        auto REF = IDEProblem.getReturnEdgeFunction(CallSite, Callee, ExitStmt,
                                                    Fact, RetSite, OutFact);
        Out.insert({OutFact, REF->computeTarget(Value)});
      }
    }
    return Out;
  }

  BitVectorSet<MonoFact>
  callToRetFlow(N CallSite, N RetSite, std::set<F> Callees,
                const BitVectorSet<MonoFact> &In) override {
    auto CTRFF =
        IDEProblem.getCallToRetFlowFunction(CallSite, RetSite, Callees);
    for (auto &[Fact, Value] : In.getAsSet()) {
      auto OutFacts = applyIFDSFlowFunction(In, CFF.get());
      for (auto &OutFact : OutFacts) {
        auto REF = IDEProblem.getCallToRetEdgeFunction(CallSite, Fact, RetSite,
                                                       OutFact, Callees);
        Out.insert({OutFact, REF->computeTarget(Value)});
      }
    }
    return Out;
  }
};

} // namespace psr

#endif
