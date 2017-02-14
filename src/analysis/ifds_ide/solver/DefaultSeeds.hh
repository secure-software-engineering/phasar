/*
 * DefaultSeeds.hh
 *
 *  Created on: 03.11.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_DEFAULTSEEDS_HH_
#define ANALYSIS_IFDS_IDE_SOLVER_DEFAULTSEEDS_HH_

#include <map>
#include <set>
#include <llvm/IR/Instruction.h>

using namespace std;

class DefaultSeeds {
public:
	template<class N, class D>
	static map<N, set<D>> make(vector<N> instructions, D zeroNode)
	{
		map<N, set<D>> res;
		for (const N& n : instructions)
			res.insert({n, set<D>{zeroNode}});
		return res;
	}
};


#endif /* ANALYSIS_IFDS_IDE_SOLVER_DEFAULTSEEDS_HH_ */
