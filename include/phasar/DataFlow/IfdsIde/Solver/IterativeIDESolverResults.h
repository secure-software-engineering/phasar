#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_ITERATIVEIDESOLVERRESULTS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_ITERATIVEIDESOLVERRESULTS_H

/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/Compressor.h"
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Table.h"
#include "phasar/Utils/TableWrappers.h"

#include "llvm/ADT/ArrayRef.h"

#include <type_traits>

namespace psr::detail {
template <typename N, typename D, typename L> class IterativeIDESolverResults {
  using inner_map_t = std::conditional_t<
      std::is_same_v<L, BinaryDomain>,
      DummyDenseTable1d<uint32_t, typename ValCompressorTraits<L>::id_type>,
      DenseTable1d<uint32_t, typename ValCompressorTraits<L>::id_type>>;

public:
  using n_t = N;
  using d_t = D;
  using l_t = L;

  typename NodeCompressorTraits<n_t>::type NodeCompressor;
  Compressor<d_t> FactCompressor;
  [[no_unique_address]] typename ValCompressorTraits<l_t>::type ValCompressor;
  llvm::SmallVector<inner_map_t, 0> ValTab;

  using ValTab_value_type = typename decltype(ValTab)::value_type;

  static_assert(std::is_same_v<ValTab_value_type, inner_map_t>);

  auto ValTab_cellVec() const {
    std::vector<typename Table<n_t, d_t, l_t>::Cell> Ret;
    Ret.reserve(ValTab.size());

    for (const auto &[M1, Inst] : llvm::zip(ValTab, NodeCompressor)) {
      for (ByConstRef<typename ValTab_value_type::value_type> M2 : M1.cells()) {
        Ret.emplace_back(Inst, FactCompressor[M2.first],
                         ValCompressor[M2.second]);
      }
    }
    return Ret;
  }
};

template <typename AnalysisDomainTy>
using IterativeIDESolverResults_P =
    IterativeIDESolverResults<typename AnalysisDomainTy::n_t,
                              typename AnalysisDomainTy::d_t,
                              typename AnalysisDomainTy::l_t>;
} // namespace psr::detail

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_ITERATIVEIDESOLVERRESULTS_H
