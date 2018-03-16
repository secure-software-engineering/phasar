/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef MWAIFDSSOLVER_H_
#define MWAIFDSSOLVER_H_

#include "../../misc/BinaryDomain.h"
#include "../../misc/SummaryStrategy.h"
#include "IFDSToIDETabulationProblem.h"
#include "MWAIDESolver.h"
#include <set>

template <typename N, typename D, typename M, typename I>
class MWAIFDSSolver : public MWAIDESolver<N, D, M, BinaryDomain, I> {
public:
  MWAIFDSSolver(IFDSTabulationProblem<N, D, M, I> &ifdsProblem,
                enum SummaryGenerationStrategy S)
      : MWAIDESolver<N, D, M, BinaryDomain, I>(ifdsProblem, S) {}

  virtual ~MWAIFDSSolver() = default;
};

#endif
