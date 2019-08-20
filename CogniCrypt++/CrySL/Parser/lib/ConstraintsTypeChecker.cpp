#include <CrySLTypechecker.h>
#include <PositionHelper.h>
#include <Types/Type.h>
#include <iostream>

namespace CCPP {
using namesapce std;
struct ConstraintsTypeChecker {
  const unordered_map<string, shared_ptr<Type>> &DefinedObjects;
  ConstraintsTypeChecker(
      unordered_map<string, shared_ptr<Type>> &DefinedObjects)
      : DefinedObjects(DefinedObjects) {}

  shared_ptr<Type> typecheck(CrySLParser::ConstrContext *constr) {
    // TODO typecheck constraints
    auto sub_constrs = constr->constr();
    if (sub_constrs.size() == 2) {
      if (constr->implies) {
        // TODO: both types must be boolean
      }
    }
  }
  bool typecheck(CrySLParser::ConstraintsContext *constr) {
    bool succ = true;
    for (auto c : constr->constr()) {
      auto ty = typecheck(c);
      if (ty.get() == nullptr ||
          ty->getPrimitiveType() != Type::PrimitiveType::BOOL) {
        succ = false;
        if (ty.get()) {
          cerr << Position(constr) << ": The constraint is not boolean" << endl;
        }
      }
    }
    return succ;
  }

}

bool CrySLTypechecker::CrySLSpec::typecheck(
    CrySLParser::ConstraintsContext *constr) {
  ConstraintsTypeChecker ctc(DefinedObjects);
  return ctc.typecheck(constr);
}
} // namespace CCPP