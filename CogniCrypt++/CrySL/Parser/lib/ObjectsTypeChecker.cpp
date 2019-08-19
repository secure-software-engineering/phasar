#include "CrySLTypechecker.h"
#include "PositionHelper.h"

namespace CCPP {
using namespace std;

bool typecheck(CrySLParser::ObjectDeclContext *decl) {
  auto name = decl->Ident()->getText();
  auto typeName = decl->typeName()->getText();
  if (DefinedObjects.count(name)) {
    cerr << Position(decl) << ": An object with the name '" << name
         << "' is already defined" << endl;
    return false;
  } else {
    DefinedObjects[name] = typeName;
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