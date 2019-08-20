#include <CrySLTypechecker.h>
#include <PositionHelper.h>
#include <TypeParser.h>

namespace CCPP {
using namespace std;

bool typecheck(std::unordered_map<std::string, std::shared_ptr<Type>>
                   &DefinedObjects CrySLParser::ObjectDeclContext *decl) {
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
    succ &= typecheck(this->DefinedObjects, decl);
  }
  return succ;
}
} // namespace CCPP