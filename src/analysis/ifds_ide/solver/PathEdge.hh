/*
 * PathEdge.hh
 *
 *  Created on: 16.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_PATHEDGE_HH_
#define ANALYSIS_IFDS_IDE_SOLVER_PATHEDGE_HH_

#include <iostream>

using namespace std;

template<typename N, typename D>
class PathEdge {
private:
	const N target;
	const D dSource, dTarget;
public:
	PathEdge(D dSource, N target, D dTarget) : target(target), dSource(dSource), dTarget(dTarget) {}

	virtual ~PathEdge() = default;

	N getTarget() { return target; }

	D factAtSource() { return dSource; }

	D factAtTarget() { return dTarget; }

	friend ostream& operator<< (ostream& os, const PathEdge& pathEdge)
	{
		return os << "<" << pathEdge.dSource << "> -> <" << pathEdge.target << "," << pathEdge.dTarget << ">";
	}

};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_PATHEDGE_HH_ */
