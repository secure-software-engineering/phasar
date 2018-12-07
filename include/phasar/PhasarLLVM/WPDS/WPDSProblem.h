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
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions.h>
#include <phasar/PhasarLLVM/IfdsIde/JoinLattice.h>
#include <phasar/PhasarLLVM/Utils/Printer.h>
#include <phasar/PhasarLLVM/WPDS/WPDSOptions.h>

namespace psr {

template <typename N, typename D, typename M, typename V, typename I>
class WPDSProblem : public FlowFunctions<N, D, M>,
                    public EdgeFunctions<N, D, M, V>,
                    public JoinLattice<V>,
                    public NodePrinter<N>,
                    public DataFlowFactPrinter<D>,
                    public MethodPrinter<M>,
                    public ValuePrinter<V> {
private:
  I ICFG;
  WPDSType WPDSTy;
  SearchDirection Direction;
  std::vector<N> Stack;
  bool Witnesses;

public:
  WPDSProblem(I ICFG, WPDSType WPDS, SearchDirection Direction,
              std::vector<N> Stack = {}, bool Witnesses = false)
      : ICFG(ICFG), WPDSTy(WPDS), Direction(Direction), Stack(move(Stack)),
        Witnesses(Witnesses) {}
  virtual ~WPDSProblem() = default;
  I interproceduralCFG() { return ICFG; }
  virtual D zeroValue() = 0;
  SearchDirection getSearchDirection() { return Direction; }
};

} // namespace psr

#endif
