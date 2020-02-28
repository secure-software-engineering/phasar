/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_VARICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_VARICFG_H_

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/VarCFG.h>

namespace psr {

template <typename N, typename M, typename C>
class VarICFG : public virtual VarCFG<N, M, C>, public virtual ICFG<N, M> {
public:
  ~VarICFG() override = default;
};

} // namespace psr

#endif
