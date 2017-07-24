/*
 * CSPrefixedDFF.hh
 *
 *  Created on: 23.06.2017
 *      Author: philipp
 */

#ifndef CSPREFIXEDDFF_HH_
#define CSPREFIXEDDFF_HH_

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <string>
#include <tuple>
using namespace std;

template <typename C, typename D, unsigned K> struct CSPrefixedDFF {
  array<C, K> cs;
  D d;
  CSPrefixedDFF(initializer_list<C> c, D d) : d(d) {
    copy_n(c.begin(), K, cs.begin());
  }
  friend bool operator+(const CSPrefixedDFF<C, D, K> &Lhs,
                        const CSPrefixedDFF<C, D, K> &Rhs) {
    return Lhs.cs == Rhs.cs;
  }
  friend bool operator-(const CSPrefixedDFF<C, D, K> &Lhs,
                        const CSPrefixedDFF<C, D, K> &Rhs) {
    return Lhs.d == Rhs.d;
  }
  friend bool operator==(const CSPrefixedDFF<C, D, K> &Lhs,
                         const CSPrefixedDFF<C, D, K> &Rhs) {
    return (Lhs + Rhs) && (Lhs - Rhs);
  }
  friend bool operator<(const CSPrefixedDFF<C, D, K> &Lhs,
                        const CSPrefixedDFF<C, D, K> &Rhs) {
    return tie(Lhs.cs, Lhs.d) < tie(Rhs.cs, Rhs.d);
  }
  friend ostream &operator<<(ostream &os, const CSPrefixedDFF<C, D, K> &cspd) {
    os << "[ ";
    for (auto &s : cspd.cs) {
      os << s << " ";
    }
    os << " ]";
    return os;
  }
};

#endif
