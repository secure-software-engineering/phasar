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

#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Utils/Printer.h>
#include <phasar/Utils/BitVectorSet.h>

namespace psr {

struct HasNoConfigurationType;

class ProjectIRDB;
template <typename T, typename F> class TypeHierarchy;
template <typename V, typename N> class PointsToInfo;
template <typename N, typename F> class CFG;

template <typename N, typename D, typename F, typename T, typename V,
          typename C>
class IntraMonoProblem : public NodePrinter<N>,
                         public DataFlowFactPrinter<D>,
                         public FunctionPrinter<F> {
  static_assert(std::is_base_of_v<CFG<N, F>, C>,
                "C must implement the CFG interface!");

protected:
  const ProjectIRDB *IRDB;
  const TypeHierarchy<T, F> *TH;
  const C *CF;
  const PointsToInfo<V, N> *PT;
  std::set<std::string> EntryPoints;

public:
  // denote that a problem does not require a configuration (type/file)
  // a user problem can override the type of configuration to be used, if any
  using ConfigurationTy = HasNoConfigurationType;

  IntraMonoProblem(const ProjectIRDB *IRDB, const TypeHierarchy<T, F> *TH,
                   const C *CF, const PointsToInfo<V, N> *PT,
                   std::set<std::string> EntryPoints = {})
      : IRDB(IRDB), TH(TH), CF(CF), PT(PT), EntryPoints(EntryPoints) {}
  ~IntraMonoProblem() override = default;

  virtual BitVectorSet<D> join(const BitVectorSet<D> &Lhs,
                               const BitVectorSet<D> &Rhs) = 0;

  virtual bool sqSubSetEqual(const BitVectorSet<D> &Lhs,
                             const BitVectorSet<D> &Rhs) = 0;

  virtual BitVectorSet<D> normalFlow(N S, const BitVectorSet<D> &In) = 0;

  virtual std::unordered_map<N, BitVectorSet<D>> initialSeeds() = 0;

  std::set<std::string> getEntryPoints() const { return EntryPoints; }

  const ProjectIRDB *getProjectIRDB() const { return IRDB; }

  const TypeHierarchy<T, F> *getTypeHierarchy() const { return TH; }

  const C *getCFG() const { return CF; }

  const PointsToInfo<V, N> *getPointstoInfo() const { return PT; }
};

} // namespace psr

#endif
