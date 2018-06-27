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

#pragma once

#include "CallString.h"
#include <iostream>

namespace psr {

template <typename D> struct CallStringPrefixedDFF {
  D d;
  CallString<D, 3> d_callstd::string;
  CallStringPrefixedDFF(D d, CallString<D, 3> cs) : d(d), d_callstd::string(cs) {}
  friend std::ostream &operator<<(std::ostream &os, const CallStringPrefixedDFF &d) {
    return os << "[ " << d.d_callstd::string << " ] - " << d.d;
  }
};

} // namespace psr
