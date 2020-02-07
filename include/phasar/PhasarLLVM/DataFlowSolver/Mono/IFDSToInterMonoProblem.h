/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Martin Mory and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_MONO_IFDSTOMONOPROBLEM_H_
#define PHASAR_PHASARLLVM_MONO_IFDSTOMONOPROBLEM_H_

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h>

namespace psr {

template <typename N, typename D, typename F, typename T, typename V,
          typename I>
class IFDSToMonoProblem : public InterMonoProblem<N, D, F, T, V, I> {
private:
  IFDSTabulationProblem<N, D, F, T, V, I> &IFDSProblem;

  BitVectorSet<D> applyIFDSFlowFunction(const BitVectorSet<D> &In,
                                        FlowFunction<D> *FF) {
    BitVectorSet<D> Out;
    for (auto &FlowFact : In.getAsSet()) {
      auto FlowFacts = FF->computeTargets(FlowFact);
      Out.insert(FlowFacts.begin(), FlowFacts.end());
    }
    return Out;
  }

public:
  IFDSToMonoProblem(IFDSTabulationProblem<N, D, F, T, V, I> &IFDSProblem)
      : IFDSProblem(IFDSProblem) {}

//   IFDSToMonoProblem(IFDSTabulationProblem &&IFDSProblem)
//       : IFDSProblem(IFDSProblem) {}

  IFDSToMonoProblem(const IFDSToMonoProblem &copy) = delete;
  IFDSToMonoProblem(IFDSToMonoProblem &&move) = delete;
  IFDSToMonoProblem &operator=(const IFDSToMonoProblem &copy) = delete;
  IFDSToMonoProblem &operator=(IFDSToMonoProblem &&move) = delete;

  BitVectorSet<D> join(const BitVectorSet<D> &Lhs,
                       const BitVectorSet<D> &Rhs) override {
    return Lhs.setUnion(Rhs);
  }

  bool sqSubSetEqual(const BitVectorSet<D> &Lhs,
                     const BitVectorSet<D> &Rhs) override {
    return Rhs.includes(Lhs);
  }

  std::unordered_map<N, BitVectorSet<D>> initialSeeds() override {
    auto Seeds = IFDSProblem.initialSeeds();
    std::unordered_map<N, BitVectorSet<D>> MonoSeeds;
    for (auto &[Inst, Facts] : Seeds) {
      MonoSeeds[Inst] = BitVectorSet<D>(Facts);
    }
    return Seeds;
  }

  BitVectorSet<D> normalFlow(N S, const BitVectorSet<D> &In) override {
    auto NFF = IFDSProblem.getNormalFlowFunction(S, N{});
    return applyIFDSFlowFunction(In, NFF.get());
  }

  BitVectorSet<D> callFlow(N CallSite, F Callee,
                           const BitVectorSet<D> &In) override {
    auto CFF = IFDSProblem.getCallFlowFunction(CallSite, Callee);
    return applyIFDSFlowFunction(In, CFF.get());
  }

  BitVectorSet<D> returnFlow(N CallSite, F Callee, N ExitStmt, N RetSite,
                             const BitVectorSet<D> &In) override {
    auto RFF =
        IFDSProblem.getRetFlowFunction(CallSite, Callee, ExitStmt, RetSite);
    return applyIFDSFlowFunction(In, RFF.get());
  }

  BitVectorSet<D> callToRetFlow(N CallSite, N RetSite, std::set<F> Callees,
                                const BitVectorSet<D> &In) override {
    auto CTRFF =
        IFDSProblem.getCallToRetFlowFunction(CallSite, RetSite, Callees);
    return applyIFDSFlowFunction(In, CTRFF.get());
  }
};

} // namespace psr

#endif
