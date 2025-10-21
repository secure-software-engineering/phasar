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

#ifndef PHASAR_DATAFLOW_MONO_INTRAMONOPROBLEM_H
#define PHASAR_DATAFLOW_MONO_INTRAMONOPROBLEM_H

#include "phasar/ControlFlow/CFGBase.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Pointer/AliasInfo.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/Soundness.h"

#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace psr {

struct HasNoConfigurationType;

template <typename T, typename F> class TypeHierarchy;
template <typename N, typename F> class CFG;

/// \brief The analysis problem interface for intraprocedural monotone problems
/// (solvable by the IntraMonoSolver). Create a subclass from this and override
/// all pure-virtual functions to create your own mono analysis.
template <typename AnalysisDomainTy> class IntraMonoProblem {
public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using c_t = typename AnalysisDomainTy::c_t;
  using db_t = typename AnalysisDomainTy::db_t;
  using mono_container_t = typename AnalysisDomainTy::mono_container_t;

  using ProblemAnalysisDomain = AnalysisDomainTy;

protected:
  const ProjectIRDBBase<db_t> *IRDB;
  const TypeHierarchy<t_t, f_t> *TH;
  const CFGBase<c_t> *CF;
  AliasInfoRef<v_t, n_t> PT;
  std::vector<std::string> EntryPoints;
  [[maybe_unused]] Soundness S = Soundness::Soundy;

public:
  using ConfigurationTy = HasNoConfigurationType;

  /// An intraprocedural monotone problem generated from an intermediate
  /// representation, a type hierarchy of said representation, a control flow
  /// graph, points-to information and optionally a vector of entry points.
  /// @param[in] IRDB A project IR database.
  /// @param[in] TH A type hierarchy based on the given IRDB.
  /// @param[in] CF A control flow graph based on the given IRDB.
  /// @param[in] PT Points-to information based on the given IRDB.
  /// @param[in] EntryPoints A vector of entry points. Provide at least one.
  IntraMonoProblem(const ProjectIRDBBase<db_t> *IRDB,
                   const TypeHierarchy<t_t, f_t> *TH, const CFGBase<c_t> *CF,
                   AliasInfoRef<v_t, n_t> PT,
                   std::vector<std::string> EntryPoints = {})
      : IRDB(IRDB), TH(TH), CF(CF), PT(PT),
        EntryPoints(std::move(EntryPoints)) {}

  virtual ~IntraMonoProblem() = default;

  virtual mono_container_t normalFlow(n_t Inst, const mono_container_t &In) = 0;

  virtual mono_container_t merge(const mono_container_t &Lhs,
                                 const mono_container_t &Rhs) = 0;

  virtual bool equal_to( // NOLINT - this would break client analyses
      const mono_container_t &Lhs, const mono_container_t &Rhs) = 0;

  virtual mono_container_t allTop() { return mono_container_t{}; }

  virtual std::unordered_map<n_t, mono_container_t> initialSeeds() = 0;

  [[nodiscard]] const std::vector<std::string> &getEntryPoints() const {
    return EntryPoints;
  }

  [[nodiscard]] const ProjectIRDBBase<db_t> *getProjectIRDB() const {
    return IRDB;
  }

  [[nodiscard]] const TypeHierarchy<t_t, f_t> *getTypeHierarchy() const {
    return TH;
  }

  [[nodiscard]] const CFGBase<c_t> *getCFG() const { return CF; }

  [[nodiscard]] AliasInfoRef<v_t, n_t> getPointstoInfo() const { return PT; }

  virtual bool setSoundness(Soundness /*S*/) { return false; }

  virtual void printContainer(llvm::raw_ostream &OS, mono_container_t C) const {
  }
};

} // namespace psr

#endif
