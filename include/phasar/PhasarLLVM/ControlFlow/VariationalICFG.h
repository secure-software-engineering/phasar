/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_VARIATIONALICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_VARIATIONALICFG_H_

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/VariationalCFG.h>

namespace psr {

template <typename N, typename M, typename C>
class VariationalICFG : public virtual VariationalCFG<N, M, C>,
                        public virtual ICFG<N, M> {
public:
    ~VariationalICFG() override = default;
};

} // namespace psr

#endif
