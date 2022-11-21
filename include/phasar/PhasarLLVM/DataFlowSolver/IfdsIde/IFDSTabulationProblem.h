/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSTABULATIONPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSTABULATIONPROBLEM_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"

#include <set>
#include <string>

namespace psr {

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IFDSTabulationProblem
    : public IDETabulationProblem<WithBinaryValueDomain<AnalysisDomainTy>,
                                  Container> {
  using Base =
      IDETabulationProblem<WithBinaryValueDomain<AnalysisDomainTy>, Container>;

public:
  using typename Base::d_t;
  using typename Base::EdgeFunctionPtrType;
  using typename Base::f_t;
  using typename Base::i_t;
  using typename Base::l_t;
  using typename Base::n_t;
  using typename Base::ProblemAnalysisDomain;
  using typename Base::t_t;
  using typename Base::v_t;

  explicit IFDSTabulationProblem(const ProjectIRDB *IRDB,
                                 std::vector<std::string> EntryPoints,
                                 d_t ZeroValue)
      : Base(IRDB, std::move(EntryPoints), std::move(ZeroValue)) {}
  ~IFDSTabulationProblem() override = default;

  EdgeFunctionPtrType getNormalEdgeFunction(n_t /*Curr*/, d_t /*CurrNode*/,
                                            n_t /*Succ*/,
                                            d_t /*SuccNode*/) override {
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  EdgeFunctionPtrType getCallEdgeFunction(n_t /*CallInst*/, d_t /*SrcNode*/,
                                          f_t /*CalleeFun*/,
                                          d_t /*DestNode*/) override {
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  EdgeFunctionPtrType getReturnEdgeFunction(n_t /*CallSite*/, f_t /*CalleeFun*/,
                                            n_t /*ExitInst*/, d_t /*ExitNode*/,
                                            n_t /*RetSite*/,
                                            d_t /*RetNode*/) override {
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  EdgeFunctionPtrType
  getCallToRetEdgeFunction(n_t /*CallSite*/, d_t /*CallNode*/, n_t /*RetSite*/,
                           d_t /*RetSiteNode*/,
                           llvm::ArrayRef<f_t> /*Callees*/) override {
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  EdgeFunctionPtrType getSummaryEdgeFunction(n_t /*Curr*/, d_t /*CurrNode*/,
                                             n_t /*Succ*/,
                                             d_t /*SuccNode*/) override {
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  BinaryDomain topElement() override { return BinaryDomain::TOP; }

  BinaryDomain bottomElement() override { return BinaryDomain::BOTTOM; }

  BinaryDomain join(BinaryDomain Lhs, BinaryDomain Rhs) override {
    if (Lhs == BinaryDomain::TOP && Rhs == BinaryDomain::TOP) {
      return BinaryDomain::TOP;
    }
    return BinaryDomain::BOTTOM;
  }

  EdgeFunctionPtrType allTopFunction() override {
    static EdgeFunctionPtrType AllTopFn =
        std::make_shared<AllTop<BinaryDomain>>(BinaryDomain::TOP);
    return AllTopFn;
  }

  void printEdgeFact(llvm::raw_ostream &OS, BinaryDomain Val) const override {
    OS << Val;
  }
};
} // namespace psr

#endif
