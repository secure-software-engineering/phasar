/*
 * Summary.hh
 *
 *  Created on: 26.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARY_HH_
#define SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARY_HH_

#include <string>
using namespace std;

template<typename N, typename D>
class IFDSSummary {
private:
	string FunctionName;
	N Start;
	N End;
//	vector<D> Inputs;
//	vector<bool> Context;
//	set<D> Outputs;

public:
	IFDSSummary();
	virtual ~IFDSSummary();
};

#endif /* SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARY_HH_ */
