
/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_STRINGIDLESS_H_
#define PHASAR_UTILS_STRINGIDLESS_H_

#include <string>

namespace psr {
struct StringIDLess {
  bool operator()(const std::string &LHS, const std::string &RHS) const;
};
} // namespace psr

#endif // PHASAR_UTILS_STRINGIDLESS_H_
