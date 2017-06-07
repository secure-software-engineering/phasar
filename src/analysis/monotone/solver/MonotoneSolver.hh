/*
 * MonotoneSolver.hh
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef MONOTONESOLVER_HH_
#define MONOTONESOLVER_HH_

#include "../CallString.hh"
#include <iostream>
#include <vector>
#include <deque>
#include <map>
#include <utility>
#include "../MonotoneProblem.hh"
using namespace std;


template <typename N, typename D, typename C>
class MonotoneSolver {
protected:
	MonotoneProblem<N,D,C>& IMProblem;
	deque<pair<N,N>> Worklist;
	map<N, set<D>> Analysis;
	void initialize() {
		// step 1: Initalization (of Worklist and Analysis)
		// add all edges to the worklist

		// set all analysis information to the empty set
		for (auto& s : set<N>()) {
			Analysis.insert({s, set()});
		}
	}

public:
	MonotoneSolver(MonotoneProblem<N,D,C>& IMP) : IMProblem(IMP) {}
	virtual ~MonotoneSolver() = default;
	virtual void solve() {
		initialize();
		// step 2: Iteration (updating Worklist and Analysis)
		while (!Worklist.empty()) {
			pair<N,N> path = Worklist.front();
			Worklist.pop_front();
			N src = path.first;
			N dst = path.second;
			set<D> Out = IMProblem.flow(src, Analysis[src]);
			if (!IMProblem.sqSubSetEq(Out, Analysis[dst])) {
				Analysis[dst] = IMProblem.joinWith(Analysis[dst], Out);
				for (auto& nprimeprime : set()) {
					Worklist.push_back({dst, nprimeprime});
				}
			}
		}
		// step 3: Presenting the result (MFP_in and MFP_out)
		// MFP_in[s] = Analysis[s];
		// MFP out[s] = IMProblem.flow(Analysis[s]);
	}
};

#endif
