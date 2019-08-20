#include "CrySLTypechecker.h"
#include "PositionHelper.h"
#include "TypeParser.h"

namespace CCPP 
{
using namespace std;
bool CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::ObjectsContext *objs) {
  bool succ = true;
  for (auto decl : objs->objectDecl()) {
    succ &= typecheck(decl);
  }
  return succ;
}
