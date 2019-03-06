/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_DEFAULTWPDSPROBLEM_H_
#define PHASAR_PHASARLLVM_WPDS_DEFAULTWPDSPROBLEM_H_

#include <vector>

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/WPDS/WPDSProblem.h>

namespace psr {

template <typename N, typename D, typename M, typename V, typename I>
class DefaultWPDSProblem : public WPDSProblem<N, D, M, V, I> {
protected:
  I ICFG;
  WPDSType WPDSTy;
  SearchDirection Direction;
  std::vector<N> Stack;
  bool Witnesses;

public:
  DefaultWPDSProblem(I ICFG, WPDSType WPDS, SearchDirection Direction,
                     std::vector<N> Stack = {}, bool Witnesses = false)
      : ICFG(ICFG), WPDSTy(WPDS), Direction(Direction), Stack(move(Stack)),
        Witnesses(Witnesses) {}
  ~DefaultWPDSProblem() override = default;
  I interproceduralCFG() override { return ICFG; }
  SearchDirection getSearchDirection() override { return Direction; }
  WPDSType getWPDSTy() override { return WPDSTy; }
  bool recordWitnesses() override { return Witnesses; }
};

} // namespace psr

#endif
