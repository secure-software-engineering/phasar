/*
 * CallString.hh
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_
#define SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_

#include <algorithm>
#include <array>
#include <initializer_list>
#include <iostream>
#include <string>
using namespace std;

template <typename T, unsigned K> class CallString {
private:
  array<T, K> callstring;

public:
  CallString() = default;

  CallString(initializer_list<T> ilist) {
    copy_n(ilist.begin(), K, callstring.begin());
  }
  friend bool operator==(const CallString<T, K> &Lhs,
                         const CallString<T, K> &Rhs) {
    return Lhs.callstring == Rhs.callstring;
  }
  friend bool operator<(const CallString<T, K> &Lhs,
                        const CallString<T, K> &Rhs) {
    return Lhs.callstring < Rhs.callstring;
  }
  friend ostream& operator<<(ostream& os, const CallString& CS) {
    for (const auto &S : CS.callstring) {
      os << S << ", ";
    }
    return os;
  }
};

#endif /* SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_ */
