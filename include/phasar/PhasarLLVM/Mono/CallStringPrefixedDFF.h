/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * CallStringPrefixedDFF.h
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_MONOTONE_CALLSTRINGPREFIXEDDFF_H_
#define SRC_ANALYSIS_MONOTONE_CALLSTRINGPREFIXEDDFF_H_

#include <iostream>
#include <phasar/PhasarLLVM/Mono/CallString.h>

namespace psr {

template <typename D> struct CallStringPrefixedDFF {
  D d;
  CallString<D, 3> d_callstring;
  CallStringPrefixedDFF(D d, CallString<D, 3> cs) : d(d), d_callstring(cs) {}
  friend std::ostream &operator<<(std::ostream &os,
                                  const CallStringPrefixedDFF &d) {
    return os << "[ " << d.d_callstring << " ] - " << d.d;
  }
};

} // namespace psr

#endif