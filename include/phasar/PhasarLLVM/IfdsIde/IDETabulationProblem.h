/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IDETabulationProblem.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_IDETABULATIONPROBLEM_H_
#define ANALYSIS_IFDS_IDE_IDETABULATIONPROBLEM_H_

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/JoinLattice.h>
#include <memory>
#include <type_traits>

template <typename N, typename D, typename M, typename V, typename I>
class IDETabulationProblem : public IFDSTabulationProblem<N, D, M, I>,
                             public EdgeFunctions<N, D, M, V>,
                             public JoinLattice<V> {
 public:
  virtual ~IDETabulationProblem() = default;
  virtual shared_ptr<EdgeFunction<V>> allTopFunction() = 0;
  virtual string VtoString(V v) = 0;
};

#endif /* ANALYSIS_IFDS_IDE_IDETABLUATIONPROBLEM_HH_ */
