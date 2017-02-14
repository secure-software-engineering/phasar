/*
 * DefaultIDETabulationProblem.hh
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_DEFAULTIDETABULATIONPROBLEM_HH_
#define ANALYSIS_IFDS_IDE_DEFAULTIDETABULATIONPROBLEM_HH_

#include "IDETabluationProblem.hh"

template<class N, class D, class M, class V, class I>
class DefaultIDETabulationProblem : public IDETabluationProblem<N,D,M,V,I> {
protected:
	I icfg;
	virtual D createZeroValue() = 0;
	D zerovalue;

public:
	DefaultIDETabulationProblem(I icfg) : icfg(icfg) {}

	virtual ~DefaultIDETabulationProblem() = default;

	I interproceduralCFG() override
	{
		return icfg;
	}

	D zeroValue() override
	{
		return zerovalue;
	}

	inline bool followReturnsPastSeeds() override
	{
		return false;
	}

	inline bool autoAddZero() override
	{
		return true;
	}

	inline bool computeValues() override
	{
		return true;
	}

	inline bool recordEdges() override
	{
		return false;
	}

};

#endif /* ANALYSIS_IFDS_IDE_DEFAULTIDETABULATIONPROBLEM_HH_ */
