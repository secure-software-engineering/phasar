#pragma once
#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/VariationalCFG.h>

namespace psr {
template <typename N, typename M, typename C>
class VariationalICFG : public virtual VariationalCFG<N, M, C>,
                        public virtual ICFG<N, M> {};
} // namespace psr