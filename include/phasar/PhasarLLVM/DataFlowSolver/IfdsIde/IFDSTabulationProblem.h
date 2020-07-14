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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_IFDSTABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_IFDSIDE_IFDSTABULATIONPROBLEM_H_

#include <iostream>
#include <map>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSIDESolverConfig.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/SolverResults.h"
#include "phasar/PhasarLLVM/Utils/Printer.h"
#include "phasar/Utils/SoundnessFlag.h"

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
  [[maybe_unused]] SoundnessFlag SF = SoundnessFlag::UNUSED;

public:
  using ConfigurationTy = HasNoConfigurationType;

  IFDSTabulationProblem(const ProjectIRDB *IRDB,
                        const TypeHierarchy<t_t, f_t> *TH, const i_t *ICF,
                        PointsToInfo<v_t, n_t> *PT,
                        std::set<std::string> EntryPoints = {})
      : IRDB(IRDB), TH(TH), ICF(ICF), PT(PT),
        EntryPoints(std::move(EntryPoints)) {}

  ~IFDSTabulationProblem() override = default;

  virtual d_t createZeroValue() const = 0;

  virtual bool isZeroValue(d_t d) const = 0;

  virtual std::map<n_t, std::set<d_t>> initialSeeds() = 0;

  d_t getZeroValue() const { return ZeroValue; }

  [[nodiscard]] std::set<std::string> getEntryPoints() const {
    return EntryPoints;
  }

  const ProjectIRDB *getProjectIRDB() const { return IRDB; }

  const TypeHierarchy<t_t, f_t> *getTypeHierarchy() const { return TH; }

  const i_t *getICFG() const { return ICF; }

  PointsToInfo<v_t, n_t> *getPointstoInfo() const { return PT; }

  void setIFDSIDESolverConfig(IFDSIDESolverConfig Config) {
    SolverConfig = Config;
  }

  [[nodiscard]] IFDSIDESolverConfig &getIFDSIDESolverConfig() {
    return SolverConfig;
  }

  virtual void emitTextReport(const SolverResults<n_t, d_t, BinaryDomain> &SR,
                              std::ostream &OS = std::cout) {
    OS << "No text report available!\n";
  }

  virtual void
  emitGraphicalReport(const SolverResults<n_t, d_t, BinaryDomain> &SR,
                      std::ostream &OS = std::cout) {
    OS << "No graphical report available!\n";
  }

  virtual bool setSoundnessFlag(SoundnessFlag SF) { return false; }
};
} // namespace psr

#endif
