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

#ifndef PHASAR_PHASARLLVM_MONO_CONTEXTS_CALLSTRINGPREFIXEDDFF_H_
#define PHASAR_PHASARLLVM_MONO_CONTEXTS_CALLSTRINGPREFIXEDDFF_H_

#include <ostream>

#include <phasar/PhasarLLVM/Mono/Contexts/CallString.h>

namespace psr {

template <typename N, typename D> struct CallStringPrefixedDFF {
  D d;
  CallString<N, D, 3> d_callstd::string;
  CallStringPrefixedDFF(D d, CallString<N, D, 3> cs)
      : d(d), d_callstd::string(cs) {}
  friend std::ostream &operator<<(std::ostream &os,
                                  const CallStringPrefixedDFF &d) {
    return os << "[ " << d.d_callstd::string << " ] - " << d.d;
  }
};

} // namespace psr

#endif
