/*
 * IDETabulationProblem.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_IDETABULATIONPROBLEM_HH_
#define ANALYSIS_IFDS_IDE_IDETABULATIONPROBLEM_HH_

#include <type_traits>
#include <memory>
#include "../icfg/ICFG.hh"
#include "IFDSTabulationProblem.hh"
#include "EdgeFunctions.hh"
#include "JoinLattice.hh"

template<typename N, typename D, typename M, typename V, typename I>
class IDETabulationProblem : public IFDSTabulationProblem<N,D,M,I>,
							 public EdgeFunctions<N,D,M,V>,
							 public JoinLattice<V> {
public:
	virtual ~IDETabulationProblem() = default;
	virtual shared_ptr<EdgeFunction<V>> allTopFunction() = 0;
	virtual string V_to_string(V v) = 0;
};

#endif /* ANALYSIS_IFDS_IDE_IDETABLUATIONPROBLEM_HH_ */
