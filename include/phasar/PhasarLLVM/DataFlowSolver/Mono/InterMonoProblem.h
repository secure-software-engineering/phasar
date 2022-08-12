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

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_INTERMONOPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_INTERMONOPROBLEM_H

#include <set>
#include <string>
#include <type_traits>

#include "phasar/PhasarLLVM/ControlFlow/ICFGBase.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h"
#include "phasar/Utils/BitVectorSet.h"

namespace psr {

class ProjectIRDB;
template <typename T, typename F> class TypeHierarchy;
template <typename V, typename N> class PointsToInfo;
template <typename N, typename F> class ICFG;

template <typename AnalysisDomainTy>
class InterMonoProblem : public IntraMonoProblem<AnalysisDomainTy> {
public:
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using mono_container_t = typename AnalysisDomainTy::mono_container_t;

  static_assert(is_icfg_v<i_t, AnalysisDomainTy>,
                "Type parameter i_t must implement the ICFG interface!");

protected:
  const i_t *ICF;

public:
  InterMonoProblem(const ProjectIRDB *IRDB, const TypeHierarchy<t_t, f_t> *TH,
                   const i_t *ICF, const PointsToInfo<v_t, n_t> *PT,
                   std::set<std::string> EntryPoints = {})
      : IntraMonoProblem<AnalysisDomainTy>(IRDB, TH, ICF, PT, EntryPoints),
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
