/*
 * CallString.hh
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_
#define SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_

#include <algorithm>
#include <deque>
#include <iostream>
using namespace std;

template <typename T, unsigned K> class CallString {
private:
  deque<T> cs;
  static const unsigned k = K;

public:
  void push(string s) {
    if (cs.size() > k - 1) {
      cs.pop_front();
    }
    cs.push_back(s);
  }
  T returnSite() {
    if (cs.size() > 0)
      return cs.back();
    return nullptr;
  }
  void pop() {
    if (cs.size() > 0) {
      cs.pop_back();
    }
  }
  size_t size() { return cs.size(); }
  friend bool operator==(const CallString &lhs, const CallString &rhs) {
    return lhs.cs == rhs.cs;
  }
  friend bool operator!=(const CallString &lhs, const CallString &rhs) {
    return !(lhs == rhs);
  }
  friend ostream &operator<<(ostream &os, const CallString &c) {
    copy(c.cs.begin(), --c.cs.end(), std::ostream_iterator<string>(os, " * "));
    os << c.cs.back();
    return os;
  }
};

#endif /* SRC_ANALYSIS_MONOTONE_CALLSTRING_HH_ */
