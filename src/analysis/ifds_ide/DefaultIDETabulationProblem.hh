/*
 * DefaultIDETabulationProblem.hh
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_DEFAULTIDETABULATIONPROBLEM_HH_
#define ANALYSIS_IFDS_IDE_DEFAULTIDETABULATIONPROBLEM_HH_

#include "IDETabulationProblem.hh"

template<class N, class D, class M, class V, class I>
class DefaultIDETabulationProblem : public IDETabulationProblem<N,D,M,V,I> {
protected:
	I icfg;
	virtual D createZeroValue() = 0;
	D zerovalue;

public:
	DefaultIDETabulationProblem(I icfg) : icfg(icfg) {
		this->solver_config.followReturnsPastSeeds = false;
		this->solver_config.autoAddZero = true;
		this->solver_config.computeValues = true;
		this->solver_config.recordEdges = true;
		this->solver_config.computePersistedSummaries = true;
	}

	virtual ~DefaultIDETabulationProblem() = default;

	I interproceduralCFG() override
	{
		return icfg;
	}

	D zeroValue() override
	{
		return zerovalue;
	}

};

#endif /* ANALYSIS_IFDS_IDE_DEFAULTIDETABULATIONPROBLEM_HH_ */
