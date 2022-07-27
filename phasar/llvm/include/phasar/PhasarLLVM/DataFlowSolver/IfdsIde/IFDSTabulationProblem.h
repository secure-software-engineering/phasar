/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSTabulationProblem.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSTABULATIONPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSTABULATIONPROBLEM_H

#include <map>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSIDESolverConfig.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/InitialSeeds.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/SolverResults.h"
#include "phasar/PhasarLLVM/Utils/Printer.h"
#include "phasar/Utils/Soundness.h"

namespace psr {

struct HasNoConfigurationType;

class ProjectIRDB;
template <typename T, typename F> class TypeHierarchy;
template <typename V, typename N> class PointsToInfo;

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IFDSTabulationProblem
    : public virtual FlowFunctions<AnalysisDomainTy, Container>,
      public virtual NodePrinter<AnalysisDomainTy>,
      public virtual DataFlowFactPrinter<AnalysisDomainTy>,
      public virtual FunctionPrinter<AnalysisDomainTy> {
public:
  using ProblemAnalysisDomain = AnalysisDomainTy;

  using d_t = typename AnalysisDomainTy::d_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using l_t = typename AnalysisDomainTy::l_t;
  using i_t = typename AnalysisDomainTy::i_t;

  static_assert(std::is_base_of_v<ICFG<n_t, f_t>, i_t>,
                "I must implement the ICFG interface!");

protected:
  IFDSIDESolverConfig SolverConfig;
  const ProjectIRDB *IRDB;
  const TypeHierarchy<t_t, f_t> *TH;
  const i_t *ICF;
  PointsToInfo<v_t, n_t> *PT;
  d_t ZeroValue;
  std::set<std::string> EntryPoints;
  [[maybe_unused]] Soundness SF = Soundness::Unused;

public:
  using ConfigurationTy = HasNoConfigurationType;

  IFDSTabulationProblem(const ProjectIRDB *IRDB,
                        const TypeHierarchy<t_t, f_t> *TH, const i_t *ICF,
                        PointsToInfo<v_t, n_t> *PT,
                        std::set<std::string> EntryPoints = {})
      : IRDB(IRDB), TH(TH), ICF(ICF), PT(PT),
        EntryPoints(std::move(EntryPoints)) {}

  ~IFDSTabulationProblem() override = default;

  /// Returns the tautological lambda (or zero) data-flow fact.
  [[nodiscard]] virtual d_t createZeroValue() const = 0;

  /// Checks if the given data-flow fact is the special tautological lambda (or
  /// zero) fact.
  [[nodiscard]] virtual bool isZeroValue(d_t FlowFact) const = 0;

  /// Returns initial seeds to be used for the analysis. This is a mapping of
  /// statements to initial analysis facts.
  [[nodiscard]] virtual InitialSeeds<n_t, d_t, l_t> initialSeeds() = 0;

  /// Returns the special tautological lambda (or zero) fact.
  [[nodiscard]] d_t getZeroValue() const { return ZeroValue; }

  /// Returns the analysis' entry points.
  [[nodiscard]] std::set<std::string> getEntryPoints() const {
    return EntryPoints;
  }

  /// Returns the underlying IR.
  [[nodiscard]] const ProjectIRDB *getProjectIRDB() const { return IRDB; }

  /// Returns the underlying type hierarchy.
  [[nodiscard]] const TypeHierarchy<t_t, f_t> *getTypeHierarchy() const {
    return TH;
  }

  /// Returns the underlying inter-procedural control-flow graph.
  [[nodiscard]] const i_t *getICFG() const { return ICF; }

  /// Returns the underlying points-to information.
  [[nodiscard]] PointsToInfo<v_t, n_t> *getPointstoInfo() const { return PT; }

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
};
} // namespace psr

#endif
