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

#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Utils/Printer.h>

namespace psr {

struct UnsupportedConfigurationType;
class ProjectIRDB;
template <typename T, typename M> class TypeHierarchy;
template <typename V> class PointsToInfo;
template <typename N, typename M> class CFG;

template <typename N, typename D, typename M, typename T, typename V, typename C>
class IntraMonoProblem : public NodePrinter<N>,
                         public DataFlowFactPrinter<D>,
                         public MethodPrinter<M> {
  static_assert(std::is_base_of_v<CFG<N, M>, C>,
                "C must implement the CFG interface!");

protected:
  const ProjectIRDB *IRDB;
  const TypeHierarchy<T, M> *TH;
  const C *CF;
  const PointsToInfo<V> *PT;
  std::set<std::string> EntryPoints;

public:
  // denote that a problem does not require a configuration (type/file)
  // a user problem can override the type of configuration to be used, if any
  using ConfigurationTy = UnsupportedConfigurationType;

  IntraMonoProblem(const ProjectIRDB *IRDB, const TypeHierarchy<T, M> *TH,
                   const C *CF, const PointsToInfo<V> *PT,
                   std::initializer_list<std::string> EntryPoints = {})
      : IRDB(IRDB), TH(TH), CF(CF), PT(PT), EntryPoints(EntryPoints) {}
  ~IntraMonoProblem() override = default;
  virtual MonoSet<D> join(const MonoSet<D> &Lhs, const MonoSet<D> &Rhs) = 0;
  virtual bool sqSubSetEqual(const MonoSet<D> &Lhs, const MonoSet<D> &Rhs) = 0;
  virtual MonoSet<D> normalFlow(N S, const MonoSet<D> &In) = 0;
  virtual MonoMap<N, MonoSet<D>> initialSeeds() = 0;
};

} // namespace psr

#endif
