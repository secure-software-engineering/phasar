#include "CrySLTypechecker.h"
#include "PositionHelper.h"
#include "TypeParser.h"

namespace CCPP {
using namespace std;

bool typecheck(CrySLParser::ObjectDeclContext *decl) {
  auto name = decl->Ident()->getText();
  if (DefinedObjects.count(name)) {
    cerr << Position(decl) << ": An object with the name '" << name
         << "' is already defined" << endl;
    return false;
  } else {
    DefinedObjects[name] = getOrCreateType(decl->typeName());
    return true;
  }
}

bool CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::ObjectsContext *objs) {
  bool succ = true;
  for (auto decl : objs->objectDecl()) {
    succ &= typecheck(decl);
  }
  return succ;
}
} // namespace CCPP