#ifndef SCOPES_HH_
#define SCOPES_HH_

#include <map>
#include <string>
#include <iostream>
using namespace std;

enum class Scope { function, module, project };

ostream& operator<< (ostream &os, const Scope &s);

extern const map<string, Scope> StringToScope;

extern const map<Scope, string> ScopeToString;


#endif