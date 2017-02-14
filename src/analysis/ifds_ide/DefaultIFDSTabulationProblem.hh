/*
 * DefaultIFDSTabulationProblem.hh
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_DEFAULTIFDSTABULATIONPROBLEM_HH_
#define ANALYSIS_IFDS_IDE_DEFAULTIFDSTABULATIONPROBLEM_HH_

#include <type_traits>
#include "IFDSTabulationProblem.hh"
#include "FlowFunctions.hh"

template<class N, class D, class M, class I>
class DefaultIFDSTabulationProblem : public IFDSTabulationProblem<N,D,M,I> {
protected:
	I icfg;
	virtual D createZeroValue() = 0;
	D zerovalue;

public:
	DefaultIFDSTabulationProblem(I icfg) : icfg(icfg) {}

	virtual ~DefaultIFDSTabulationProblem() = default;

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

#endif /* ANALYSIS_IFDS_IDE_DEFAULTIFDSTABULATIONPROBLEM_HH_ */
