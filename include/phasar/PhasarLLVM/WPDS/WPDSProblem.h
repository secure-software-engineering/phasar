/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_WPDSPROBLEM_H_
#define PHASAR_PHASARLLVM_WPDS_WPDSPROBLEM_H_

#include <vector>

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/Utils/Printer.h>
#include <phasar/PhasarLLVM/WPDS/WPDSOptions.h>

namespace psr {

template <typename N, typename D, typename M, typename V, typename I>
class WPDSProblem : public IDETabulationProblem<N, D, M, V, I> {
private:
  WPDSType WPDSTy;
  SearchDirection Direction;
  std::vector<N> Stack;
  bool Witnesses;

public:
  WPDSProblem(I ICFG, WPDSType WPDS, SearchDirection Direction,
              std::vector<N> Stack = {}, bool Witnesses = false)
      : ICFG(ICFG), WPDSTy(WPDS), Direction(Direction), Stack(move(Stack)),
        Witnesses(Witnesses) {}
  ~WPDSProblem() override = default;
  I ICFG;
  I interproceduralCFG() override { return ICFG; }
  SearchDirection getSearchDirection() { return Direction; }
  WPDSType getWPDSTy() { return WPDSTy; }
  bool getWitnesses() { return Witnesses; }
};

} // namespace psr

#endif
