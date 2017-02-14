/*
 * TwoElementSet.hh
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_UTILS_TWOELEMENTSET_HH_
#define ANALYSIS_IFDS_IDE_UTILS_TWOELEMENTSET_HH_

#include <cstddef>

template<typename E>
class TwoElementSet {
private:
	const E first, second;
public:
	TwoElementSet(E first, E second) : first(first), second(second) { };
	size_t size() { return 2; }
	virtual ~TwoElementSet() = default;
};

#endif /* ANALYSIS_IFDS_IDE_UTILS_TWOELEMENTSET_HH_ */
