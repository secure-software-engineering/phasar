/*
 * ObservedCallingContexts.hh
 *
 *  Created on: 14.06.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_OBSERVEDCALLINGCONTEXTS_HH_
#define ANALYSIS_IFDS_IDE_OBSERVEDCALLINGCONTEXTS_HH_

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
using namespace std;

class ObservedCallingContexts {
private:
	// Maps a function to the set of contexts that have been recognized so far
	map<string, set<vector<bool>>> ObservedCTX;

public:
	ObservedCallingContexts() = default;
	~ObservedCallingContexts() = default;
	void addObservedCTX(string FName, vector<bool> CTX);
	bool containsCTX(string FName);
	set<vector<bool>> getObservedCTX(string FName);
	void print();
};

#endif
