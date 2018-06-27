/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#pragma once

#include <phasar/PhasarLLVM/IfdsIde/Solver/IFDSToIDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/MWAIDESolver.h>
#include <phasar/PhasarLLVM/Utils/BinaryDomain.h>
#include <phasar/PhasarLLVM/Utils/SummaryStrategy.h>
#include <set>
namespace psr {

template <typename N, typename D, typename M, typename I>
class MWAIFDSSolver : public MWAIDESolver<N, D, M, BinaryDomain, I> {
public:
  MWAIFDSSolver(IFDSTabulationProblem<N, D, M, I> &ifdsProblem,
                enum SummaryGenerationStrategy S)
      : MWAIDESolver<N, D, M, BinaryDomain, I>(ifdsProblem, S) {}

  virtual ~MWAIFDSSolver() = default;
};

} // namespace psr
