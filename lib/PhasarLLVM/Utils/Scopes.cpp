/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>

#include "phasar/PhasarLLVM/Utils/Scopes.h"

using namespace std;
using namespace psr;

namespace psr {

const map<string, Scope> StringToScope{{"function", Scope::function},
                                       {"module", Scope::module},
                                       {"project", Scope::project}};

const map<Scope, string> ScopeToString{{Scope::function, "function"},
                                       {Scope::module, "module"},
                                       {Scope::project, "project"}};

ostream &operator<<(ostream &OS, const Scope &S) {
  return OS << ScopeToString.at(S);
}
} // namespace psr
