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

template <typename N, typename D, typename F, typename T, typename V,
          typename I>
class IFDSTabulationProblem : public virtual FlowFunctions<N, D, F>,
                              public virtual NodePrinter<N>,
                              public virtual DataFlowFactPrinter<D>,
                              public virtual FunctionPrinter<F> {
  static_assert(std::is_base_of_v<ICFG<N, F>, I>,
                "I must implement the ICFG interface!");

protected:
  IFDSIDESolverConfig SolverConfig;
  const ProjectIRDB *IRDB;
  const TypeHierarchy<T, F> *TH;
  const I *ICF;
  const PointsToInfo<V, N> *PT;
  D ZeroValue;
  std::set<std::string> EntryPoints;
  [[maybe_unused]] SoundnessFlag SF = SoundnessFlag::UNUSED;

public:
  using ConfigurationTy = HasNoConfigurationType;

  IFDSTabulationProblem(const ProjectIRDB *IRDB, const TypeHierarchy<T, F> *TH,
                        const I *ICF, const PointsToInfo<V, N> *PT,
                        std::set<std::string> EntryPoints = {})
      : IRDB(IRDB), TH(TH), ICF(ICF), PT(PT), EntryPoints(EntryPoints) {}

  ~IFDSTabulationProblem() override = default;

  virtual D createZeroValue() const = 0;

  virtual bool isZeroValue(D d) const = 0;

  virtual std::map<N, std::set<D>> initialSeeds() = 0;

  D getZeroValue() const { return ZeroValue; }

  std::set<std::string> getEntryPoints() const { return EntryPoints; }

  const ProjectIRDB *getProjectIRDB() const { return IRDB; }

  const TypeHierarchy<T, F> *getTypeHierarchy() const { return TH; }

  const I *getICFG() const { return ICF; }

  const PointsToInfo<V, N> *getPointstoInfo() const { return PT; }

  void setIFDSIDESolverConfig(IFDSIDESolverConfig Config) {
    SolverConfig = Config;
  }

  IFDSIDESolverConfig getIFDSIDESolverConfig() const { return SolverConfig; }

  virtual void emitTextReport(const SolverResults<N, D, BinaryDomain> &SR,
                              std::ostream &OS = std::cout) {
    OS << "No text report available!\n";
  }

  virtual void emitGraphicalReport(const SolverResults<N, D, BinaryDomain> &SR,
                                   std::ostream &OS = std::cout) {
    OS << "No graphical report available!\n";
  }

  virtual bool setSoundnessFlag(SoundnessFlag SF) { return false; }
};
} // namespace psr

#endif
