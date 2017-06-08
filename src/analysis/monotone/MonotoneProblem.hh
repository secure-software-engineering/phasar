/*
 * InterMonotoneProblem.hh
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef MONOTONEPROBLEM_HH_
#define MONOTONEPROBLEM_HH_

#include <set>
using namespace std;

template <typename N, typename D, typename C>
class MonotoneProblem {
protected:
	C CFG;

public:
	MonotoneProblem(C cfg) : CFG(cfg) {}
	virtual ~MonotoneProblem() = default;
	C getCFG() { return CFG; }
	virtual set<D> join(const set<D>& lhs, const set<D>& rhs) = 0;
	virtual bool sqSubSetEq(const set<D>& lhs, const set<D>& rhs) = 0;
	virtual set<D> flow(N s, const set<D>& in) = 0;
	virtual map<N, set<D>> initialSeeds() = 0;
};

#endif
