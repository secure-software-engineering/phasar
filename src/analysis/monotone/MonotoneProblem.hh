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

template <typename N, typename D, typename M, typename C>
class MonotoneProblem {
protected:
	C CFG;
	M Function;

public:
	MonotoneProblem(C cfg, M f) : CFG(cfg), Function(f) {}
	virtual ~MonotoneProblem() = default;
	C getCFG() { return CFG; }
	M getFunction() { return Function; }
	virtual set<D> join(const set<D>& lhs, const set<D>& rhs) = 0;
	virtual bool sqSubSetEq(const set<D>& lhs, const set<D>& rhs) = 0;
	virtual set<D> flow(N s, const set<D>& in) = 0;
	virtual map<N, set<D>> initialSeeds() = 0;
};

#endif
