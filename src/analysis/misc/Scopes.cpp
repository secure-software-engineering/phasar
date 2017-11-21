#include "Scopes.h"

const map<string, Scope> StringToScope{{"function", Scope::function},
                                       {"module", Scope::module},
                                       {"project", Scope::project}};

const map<Scope, string> ScopeToString{{Scope::function, "function"},
                                       {Scope::module, "module"},
                                       {Scope::project, "project"}};

ostream &operator<<(ostream &os, const Scope &s) {
  return os << ScopeToString.at(s);
}
