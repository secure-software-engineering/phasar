/*
 * SolverConfiguration.hh
 *
 *  Created on: 16.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVERCONFIGURATION_HH_
#define ANALYSIS_IFDS_IDE_SOLVERCONFIGURATION_HH_

#include <iostream>
using namespace std;

struct SolverConfiguration {
	SolverConfiguration() = default;
	virtual ~SolverConfiguration() = default;
	bool followReturnsPastSeeds = false;
	bool autoAddZero = false;
	bool computeValues = false;
	bool recordEdges = false;
	friend ostream& operator<< (ostream& os, const SolverConfiguration& sc);
};

#endif /* ANALYSIS_IFDS_IDE_SOLVERCONFIGURATION_HH_ */
