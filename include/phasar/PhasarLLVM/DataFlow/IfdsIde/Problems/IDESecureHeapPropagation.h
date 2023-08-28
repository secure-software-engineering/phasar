/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDESECUREHEAPPROPAGATION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDESECUREHEAPPROPAGATION_H

#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"

#include "llvm/ADT/StringRef.h"

namespace psr {
enum class SecureHeapFact { ZERO, INITIALIZED };
enum class SecureHeapValue { TOP, INITIALIZED, BOT };

template <> struct JoinLatticeTraits<SecureHeapValue> {
  static constexpr auto bottom() noexcept { return SecureHeapValue::BOT; }
  static constexpr auto top() noexcept { return SecureHeapValue::TOP; }
  static constexpr auto join(SecureHeapValue LHS,
                             SecureHeapValue RHS) noexcept {
    if (LHS == RHS || RHS == top()) {
      return LHS;
    }
    if (LHS == top()) {
      return RHS;
    }
    return bottom();
  }
};

struct IDESecureHeapPropagationAnalysisDomain : LLVMAnalysisDomainDefault {
  using d_t = SecureHeapFact;
  using l_t = SecureHeapValue;
};

class IDESecureHeapPropagation
    : public IDETabulationProblem<IDESecureHeapPropagationAnalysisDomain> {

  static constexpr llvm::StringLiteral InitializerFn =
      "CRYPTO_secure_malloc_init";
  static constexpr llvm::StringLiteral ShutdownFn = "CRYPTO_secure_malloc_done";

public:
  using IDETabProblemType =
      IDETabulationProblem<IDESecureHeapPropagationAnalysisDomain>;
  using typename IDETabProblemType::d_t;
  using typename IDETabProblemType::f_t;
  using typename IDETabProblemType::i_t;
  using typename IDETabProblemType::l_t;
  using typename IDETabProblemType::n_t;
  using typename IDETabProblemType::t_t;
  using typename IDETabProblemType::v_t;

  IDESecureHeapPropagation(const LLVMProjectIRDB *IRDB,
                           std::vector<std::string> EntryPoints = {"main"});

  ~IDESecureHeapPropagation() override = default;

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFunc) override;

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeMthd,
                                         n_t ExitInst, n_t RetSite) override;

  FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override;

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite,
                                             f_t DestMthd) override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  [[nodiscard]] d_t createZeroValue() const;

  [[nodiscard]] bool isZeroValue(d_t Fact) const noexcept override;

  // in addition provide specifications for the IDE parts

  EdgeFunction<l_t> getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                          d_t SuccNode) override;

  EdgeFunction<l_t> getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                        f_t DestinationMethod,
                                        d_t DestNode) override;

  EdgeFunction<l_t> getReturnEdgeFunction(n_t CallSite, f_t CalleeMethod,
                                          n_t ExitInst, d_t ExitNode,
                                          n_t RetSite, d_t RetNode) override;

  EdgeFunction<l_t>
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode,
                           llvm::ArrayRef<f_t> Callees) override;

  EdgeFunction<l_t> getSummaryEdgeFunction(n_t CallSite, d_t CallNode,
                                           n_t RetSite,
                                           d_t RetSiteNode) override;

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      llvm::raw_ostream &OS) override;
};

llvm::StringRef DToString(SecureHeapFact Fact) noexcept;
llvm::StringRef LToString(SecureHeapValue Val) noexcept;
} // namespace psr

#endif
