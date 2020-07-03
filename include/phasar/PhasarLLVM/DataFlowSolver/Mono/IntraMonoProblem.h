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

#ifndef PHASAR_PHASARLLVM_MONO_INTRAMONOPROBLEM_H_
#define PHASAR_PHASARLLVM_MONO_INTRAMONOPROBLEM_H_

#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "phasar/Config/ContainerConfiguration.h"
#include "phasar/PhasarLLVM/Utils/Printer.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/SoundnessFlag.h"

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

  static_assert(std::is_base_of_v<CFG<n_t, f_t>, c_t>,
                "c_t must implement the CFG interface!");

  using ProblemAnalysisDomain = AnalysisDomainTy;

protected:
  const ProjectIRDB *IRDB;
  const TypeHierarchy<t_t, f_t> *TH;
  const c_t *CF;
  const PointsToInfo<v_t, n_t> *PT;
  std::set<std::string> EntryPoints;
  [[maybe_unused]] SoundnessFlag SF = SoundnessFlag::UNUSED;

public:
  // denote that a problem does not require a configuration (type/file)
  // a user problem can override the type of configuration to be used, if any
  using ConfigurationTy = HasNoConfigurationType;

  IntraMonoProblem(const ProjectIRDB *IRDB, const TypeHierarchy<t_t, f_t> *TH,
                   const c_t *CF, const PointsToInfo<v_t, n_t> *PT,
                   std::set<std::string> EntryPoints = {})
      : IRDB(IRDB), TH(TH), CF(CF), PT(PT), EntryPoints(EntryPoints) {}
  ~IntraMonoProblem() override = default;

  virtual BitVectorSet<d_t> join(const BitVectorSet<d_t> &Lhs,
                                 const BitVectorSet<d_t> &Rhs) = 0;

  virtual bool sqSubSetEqual(const BitVectorSet<d_t> &Lhs,
                             const BitVectorSet<d_t> &Rhs) = 0;

  virtual BitVectorSet<d_t> normalFlow(n_t S, const BitVectorSet<d_t> &In) = 0;

  virtual std::unordered_map<n_t, BitVectorSet<d_t>> initialSeeds() = 0;

  std::set<std::string> getEntryPoints() const { return EntryPoints; }

  const ProjectIRDB *getProjectIRDB() const { return IRDB; }

  const TypeHierarchy<t_t, f_t> *getTypeHierarchy() const { return TH; }

  const c_t *getCFG() const { return CF; }

  const PointsToInfo<v_t, n_t> *getPointstoInfo() const { return PT; }

  virtual bool setSoundnessFlag(SoundnessFlag SF) { return false; }
};

} // namespace psr

#endif
