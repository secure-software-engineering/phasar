#pragma once
#include "ICFG.h"
#include "VariationalCFG.h"


template <typename N, typename M, typename C>
class VariationalICFG : public virtual VariationalCFG<N, M, C>,
                        public virtual ICFG<N, M, C> {};