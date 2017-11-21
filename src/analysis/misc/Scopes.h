#ifndef SCOPES_H_
#define SCOPES_H_

#include <iostream>
#include <map>
#include <string>
using namespace std;

enum class Scope { function, module, project };

ostream &operator<<(ostream &os, const Scope &s);

extern const map<string, Scope> StringToScope;

extern const map<Scope, string> ScopeToString;

#endif