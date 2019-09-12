#include <Parser/CrySLTypechecker.h>
#include <Parser/PositionHelper.h>
#include <Parser/Types/Type.h>
#include <iostream>
namespace CCPP {

bool CrySLTypechecker::CrySLSpec::checkPredicate(
    CrySLParser::ReqPredContext *requ) {
  if (requ->reqPredLit()) {
    return checkPredicate(requ->reqPredLit()->pred());
  }

  auto subPreds = requ->reqPred();
  bool succ = true;
  for (auto sub : subPreds) {
    succ &= checkPredicate(sub);
  }
  return succ;
}
bool CrySLTypechecker::CrySLSpec::typecheck(
    CrySLParser::RequiresBlockContext *req) {
  bool result = true;

  for (auto requ : req->reqPred()) {
    RequiredPreds.push_back(requ);
    result &= checkPredicate(requ);
  }
  return result;
}
} // namespace CCPP