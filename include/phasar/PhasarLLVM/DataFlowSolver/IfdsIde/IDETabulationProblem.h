/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IDETABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IDETABULATIONPROBLEM_H_

#include "phasar/DB/ProjectIRDBBase.h"
#include "phasar/PhasarLLVM/ControlFlow/ICFGBase.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSIDESolverConfig.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/InitialSeeds.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/JoinLattice.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/SolverResults.h"
#include "phasar/PhasarLLVM/Utils/Printer.h"
#include "phasar/Utils/Soundness.h"

#include <cassert>
#include <memory>
#include <set>
#include <string>
#include <type_traits>

namespace psr {

struct HasNoConfigurationType;

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IDETabulationProblem : public FlowFunctions<AnalysisDomainTy, Container>,
                             public NodePrinter<AnalysisDomainTy>,
                             public DataFlowFactPrinter<AnalysisDomainTy>,
                             public FunctionPrinter<AnalysisDomainTy>,
                             public EdgeFunctions<AnalysisDomainTy>,
                             public JoinLattice<AnalysisDomainTy>,
                             public EdgeFactPrinter<AnalysisDomainTy> {
public:
  using ProblemAnalysisDomain = AnalysisDomainTy;
  using d_t = typename AnalysisDomainTy::d_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using l_t = typename AnalysisDomainTy::l_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using db_t = typename AnalysisDomainTy::db_t;

  using ConfigurationTy = HasNoConfigurationType;

  explicit IDETabulationProblem(const db_t *IRDB,
                                std::vector<std::string> EntryPoints,
                                std::optional<d_t> ZeroValue)
      : IRDB(IRDB), EntryPoints(std::move(EntryPoints)),
        ZeroValue(std::move(ZeroValue)) {
    static_assert(std::is_base_of_v<ProjectIRDBBase<db_t>, db_t>,
                  "db_t must implement the ProjectIRDBBase interface!");
    assert(IRDB != nullptr);
  }

  virtual ~IDETabulationProblem() = default;

  /// Returns an edge function that represents the top element of the analysis.
  virtual EdgeFunction<l_t> allTopFunction() = 0;

  /// Checks if the given data-flow fact is the special tautological lambda (or
  /// zero) fact.
  [[nodiscard]] virtual bool isZeroValue(d_t FlowFact) const {
    assert(ZeroValue.has_value());
    return FlowFact == *ZeroValue;
  }

  /// Returns initial seeds to be used for the analysis. This is a mapping of
  /// statements to initial analysis facts.
  [[nodiscard]] virtual InitialSeeds<n_t, d_t, l_t> initialSeeds() = 0;

  /// Returns the special tautological lambda (or zero) fact.
  [[nodiscard]] d_t getZeroValue() const {
    assert(ZeroValue.has_value());
    return *ZeroValue;
  }

  void initializeZeroValue(d_t Zero) {
    assert(!ZeroValue.has_value());
    ZeroValue = std::move(Zero);
  }

  /// Sets the configuration to be used by the IFDS/IDE solver.
  void setIFDSIDESolverConfig(IFDSIDESolverConfig Config) {
    SolverConfig = Config;
  }

  /// Returns the configuration of the IFDS/IDE solver.
  [[nodiscard]] IFDSIDESolverConfig &getIFDSIDESolverConfig() {
    return SolverConfig;
  }

  /// Generates a text report of the results that is written to the specified
  /// output stream.
  virtual void
  emitTextReport([[maybe_unused]] const SolverResults<n_t, d_t, l_t> &Results,
                 llvm::raw_ostream &OS = llvm::outs()) {
    OS << "No text report available!\n";
  }

  /// Generates a graphical report, e.g. in html or other markup languages, of
  /// the results that is written to the specified output stream.
  virtual void emitGraphicalReport(
      [[maybe_unused]] const SolverResults<n_t, d_t, l_t> &Results,
      llvm::raw_ostream &OS = llvm::outs()) {
    OS << "No graphical report available!\n";
  }

  /// Sets the level of soundness to be used by the analysis. Returns false if
  /// the level of soundness is ignored. Otherwise, true.
  virtual bool setSoundness(Soundness /*S*/) { return false; }

protected:
  typename FlowFunctions<AnalysisDomainTy, Container>::FlowFunctionPtrType
  generateFromZero(d_t FactToGenerate) {
    return generateFlow(std::move(FactToGenerate), getZeroValue());
  }

  const db_t *IRDB{};
  std::vector<std::string> EntryPoints;
  std::optional<d_t> ZeroValue;

  IFDSIDESolverConfig SolverConfig{};

  [[maybe_unused]] Soundness SF = Soundness::Soundy;
};

} // namespace psr

#endif
