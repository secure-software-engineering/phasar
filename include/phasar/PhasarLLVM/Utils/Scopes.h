/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef SCOPES_H_
#define SCOPES_H_

#include <iostream>
#include <map>
#include <string>
using namespace std;

namespace psr{

enum class Scope { function, module, project };

ostream &operator<<(ostream &os, const Scope &s);

extern const map<string, Scope> StringToScope;

extern const map<Scope, string> ScopeToString;

}//namespace psr

#endif