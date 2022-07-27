/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Summary.h
 *
 *  Created on: 26.05.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSSUMMARY_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSSUMMARY_H

#include <set>
#include <vector>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"

namespace psr {

template <typename D, typename N> class IFDSSummary : FlowFunction<D> {
private:
  N StartNode;
  N EndNode;
  std::vector<bool> Context;
  std::set<D> Outputs;
  D ZeroValue;

public:
  IFDSSummary(N Start, N End, std::vector<bool> C, std::set<D> Gen, D ZV)
      : StartNode(Start), EndNode(End), Context(std::move(C)), Outputs(Gen),
        ZeroValue(ZV) {}
  virtual ~IFDSSummary() = default;
  std::set<D> computeTargets(D Source) override {
    if (Source == ZeroValue) {
      Outputs.insert(Source);
      return Outputs;
    }
    return {Source};
  }

  N getStartNode() const { return StartNode; }

  N getEndNode() const { return EndNode; }
};

} // namespace psr

#endif
