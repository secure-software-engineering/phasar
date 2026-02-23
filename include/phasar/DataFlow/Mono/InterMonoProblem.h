/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * InterMonoProblem.h
 *
 *  Created on: 23.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_DATAFLOW_MONO_INTERMONOPROBLEM_H
#define PHASAR_DATAFLOW_MONO_INTERMONOPROBLEM_H

#include "phasar/ControlFlow/ICFG.h"
#include "phasar/DataFlow/Mono/IntraMonoProblem.h"
#include "phasar/Pointer/AliasInfo.h"
#include "phasar/Utils/BitVectorSet.h"

#include <set>
#include <string>
#include <type_traits>

namespace psr {

template <typename T>
concept InterMonoAnalysisDomain = MonoAnalysisDomain<T> && requires() {
  typename T::i_t;
  requires ICFG<typename T::i_t>;
};

/// \brief The analysis problem interface for interprocedural monotone problems
/// (solvable by the InterMonoSolver). Create a subclass from this and override
/// all pure-virtual functions to create your own inter-mono analysis.
template <InterMonoAnalysisDomain AnalysisDomainTy>
class InterMonoProblem : public IntraMonoProblem<AnalysisDomainTy> {
public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using db_t = typename AnalysisDomainTy::db_t;
  using mono_container_t = typename AnalysisDomainTy::mono_container_t;

protected:
  const i_t *ICF;

public:
  /// An interprocedural monotone problem generated from an intermediate
  /// representation, a type hierarchy of said representation, a control flow
  /// graph, points-to information and optionally a vector of entry points.
  /// @param[in] IRDB A project IR database.
  /// @param[in] TH A type hierarchy based on the given IRDB.
  /// @param[in] CF A control flow graph based on the given IRDB.
  /// @param[in] PT Points-to information based on the given IRDB.
  /// @param[in] EntryPoints A vector of entry points. Provide at least one.
  InterMonoProblem(const db_t *IRDB, const i_t *ICF, AliasInfoRef<v_t, n_t> PT,
                   std::vector<std::string> EntryPoints = {})
      : IntraMonoProblem<AnalysisDomainTy>(IRDB, ICF, PT, EntryPoints),
        ICF(ICF) {}

  ~InterMonoProblem() override = default;
  InterMonoProblem(const InterMonoProblem &Other) = delete;
  InterMonoProblem(InterMonoProblem &&Other) = delete;
  InterMonoProblem &operator=(const InterMonoProblem &Other) = delete;
  InterMonoProblem &operator=(InterMonoProblem &&Other) = delete;

  virtual mono_container_t callFlow(n_t CallSite, f_t Callee,
                                    const mono_container_t &In) = 0;

  virtual mono_container_t returnFlow(n_t CallSite, f_t Callee, n_t ExitStmt,
                                      n_t RetSite,
                                      const mono_container_t &In) = 0;

  virtual mono_container_t callToRetFlow(n_t CallSite, n_t RetSite,
                                         llvm::ArrayRef<f_t> Callees,
                                         const mono_container_t &In) = 0;

  [[nodiscard]] const i_t *getICFG() const { return ICF; }
};

} // namespace psr

#endif
