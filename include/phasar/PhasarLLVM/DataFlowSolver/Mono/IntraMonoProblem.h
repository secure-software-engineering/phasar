/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IntraMonoProblem.h
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_INTRAMONOPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_INTRAMONOPROBLEM_H

#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "phasar/PhasarLLVM/ControlFlow/CFGBase.h"
#include "phasar/PhasarLLVM/Utils/Printer.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/Soundness.h"

namespace psr {

struct HasNoConfigurationType;

class ProjectIRDB;
template <typename T, typename F> class TypeHierarchy;
template <typename V, typename N> class PointsToInfo;
template <typename N, typename F> class CFG;

template <typename AnalysisDomainTy>
class IntraMonoProblem : public NodePrinter<AnalysisDomainTy>,
                         public DataFlowFactPrinter<AnalysisDomainTy>,
                         public FunctionPrinter<AnalysisDomainTy> {
public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using c_t = typename AnalysisDomainTy::c_t;
  using mono_container_t = typename AnalysisDomainTy::mono_container_t;

  static_assert(is_cfg_v<c_t, AnalysisDomainTy>,
                "c_t must implement the CFG interface!");

  using ProblemAnalysisDomain = AnalysisDomainTy;

protected:
  const ProjectIRDB *IRDB;
  const TypeHierarchy<t_t, f_t> *TH;
  const c_t *CF;
  const PointsToInfo<v_t, n_t> *PT;
  std::set<std::string> EntryPoints;
  [[maybe_unused]] Soundness S = Soundness::Soundy;

public:
  // denote that a problem does not require a configuration (type/file)
  // a user problem can override the type of configuration to be used, if any
  using ConfigurationTy = HasNoConfigurationType;

  IntraMonoProblem(const ProjectIRDB *IRDB, const TypeHierarchy<t_t, f_t> *TH,
                   const c_t *CF, const PointsToInfo<v_t, n_t> *PT,
                   std::set<std::string> EntryPoints = {})
      : IRDB(IRDB), TH(TH), CF(CF), PT(PT),
        EntryPoints(std::move(EntryPoints)) {}

  ~IntraMonoProblem() override = default;

  virtual mono_container_t normalFlow(n_t Inst, const mono_container_t &In) = 0;

  virtual mono_container_t merge(const mono_container_t &Lhs,
                                 const mono_container_t &Rhs) = 0;

  virtual bool equal_to( // NOLINT - this would break client analyses
      const mono_container_t &Lhs, const mono_container_t &Rhs) = 0;

  virtual mono_container_t allTop() { return mono_container_t{}; }

  virtual std::unordered_map<n_t, mono_container_t> initialSeeds() = 0;

  [[nodiscard]] std::set<std::string> getEntryPoints() const {
    return EntryPoints;
  }

  [[nodiscard]] const ProjectIRDB *getProjectIRDB() const { return IRDB; }

  [[nodiscard]] const TypeHierarchy<t_t, f_t> *getTypeHierarchy() const {
    return TH;
  }

  [[nodiscard]] const c_t *getCFG() const { return CF; }

  [[nodiscard]] const PointsToInfo<v_t, n_t> *getPointstoInfo() const {
    return PT;
  }

  virtual bool setSoundness(Soundness /*S*/) { return false; }

  virtual void printContainer(llvm::raw_ostream &OS, mono_container_t C) const {
  }
};

} // namespace psr

#endif
