/*
 * SolverConfiguration.hh
 *
 *  Created on: 16.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVERCONFIGURATION_HH_
#define ANALYSIS_IFDS_IDE_SOLVERCONFIGURATION_HH_

class SolverConfiguration {
public:
	virtual ~SolverConfiguration() = default;

	virtual bool followReturnsPastSeeds() = 0;

	virtual bool autoAddZero() = 0;

	virtual bool computeValues() = 0;

	virtual bool recordEdges() = 0;
};

#endif /* ANALYSIS_IFDS_IDE_SOLVERCONFIGURATION_HH_ */
