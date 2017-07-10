/*
 * CallString.hh
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_
#define SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_

#include <iostream>
#include <array>
#include <string>
using namespace std;

template <typename T, unsigned long K>
class CallString {
private:
	array<T, K> callstring;

public:
	CallString() {}
	friend bool operator< (const CallString<T,K>& Lhs, const CallString<T,K>& Rhs) {
		return Lhs.callstring < Rhs.callstring;
	}	
};

#endif /* SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_ */
