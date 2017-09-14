/*
 * CallStringPrefixedDFF.hh
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MONOTONE_CALLSTRINGPREFIXEDDFF_HH_
#define SRC_ANALYSIS_MONOTONE_CALLSTRINGPREFIXEDDFF_HH_

#include "CallString.hh"
#include <iostream>
using namespace std;

template <typename D> struct CallStringPrefixedDFF {
  D d;
  CallString<D, 3> d_callstring;
  CallStringPrefixedDFF(D d, CallString<D, 3> cs) : d(d), d_callstring(cs) {}
  friend ostream &operator<<(ostream &os, const CallStringPrefixedDFF &d) {
    return os << "[ " << d.d_callstring << " ] - " << d.d;
  }
};

#endif