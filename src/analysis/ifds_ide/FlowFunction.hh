/*
 * FlowFunction.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_FLOWFUNCTION_HH_
#define ANALYSIS_IFDS_IDE_FLOWFUNCTION_HH_

#include <set>

using namespace std;

template<typename D>
class FlowFunction {
public:
	virtual ~FlowFunction() = default;
	virtual set<D> computeTargets(D source) = 0;
};

#endif /* ANALYSIS_IFDS_IDE_FLOWFUNCTION_HH_ */
