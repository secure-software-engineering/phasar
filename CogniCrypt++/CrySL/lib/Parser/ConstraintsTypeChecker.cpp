#include <CrySLTypechecker.h>
#include <ErrorHelper.h>
#include <TypeParser.h>
#include <Types/Type.h>
#include <iostream>

namespace CCPP {
using namespace std;
using namespace Types;
struct ConstraintsTypeChecker {
  const unordered_map<string, shared_ptr<Type>> &DefinedObjects;
  ConstraintsTypeChecker(
      const unordered_map<string, shared_ptr<Type>> &DefinedObjects)
      : DefinedObjects(DefinedObjects) {}

  shared_ptr<const Type> typecheck(CrySLParser::ConstrContext *constr);
  shared_ptr<const Type> typecheckUnary(CrySLParser::ConstrContext *constr,
                                        CrySLParser::ConstrContext *sub_constr);
  shared_ptr<const Type>
  typecheckArithCompBinary(CrySLParser::ConstrContext *constr,
                           CrySLParser::ConstrContext *lhs,
                           CrySLParser::ConstrContext *rhs);
  bool typecheck(CrySLParser::ConstraintsContext *constr);
};

shared_ptr<const Type>
ConstraintsTypeChecker::typecheckUnary(CrySLParser::ConstrContext *constr,
                                       CrySLParser::ConstrContext *sub_constr) {
  if (constr->lnot) {
    auto subTy = typecheck(sub_constr);
    auto ret = subTy && subTy->getPrimitiveType() == Type::PrimitiveType::BOOL
                   ? subTy
                   : nullptr;
    return reportIfNull(
        constr, ret,
        "The logical negation can only be applied on a boolean value");
  } else {
    // parens
    return typecheck(sub_constr);
  }
}

shared_ptr<const Type> ConstraintsTypeChecker::typecheckArithCompBinary(
    CrySLParser::ConstrContext *constr, CrySLParser::ConstrContext *lhs,
    CrySLParser::ConstrContext *rhs) {
  auto lhsTy = typecheck(lhs);
  auto rhsTy = typecheck(rhs);

  if (!lhsTy || !rhsTy)
    return nullptr;

  if (constr->land || constr->lor) {
    // both types have to be binary
    auto ret = lhsTy->getPrimitiveType() == rhsTy->getPrimitiveType() &&
                       lhsTy->getPrimitiveType() == Type::PrimitiveType::BOOL
                   ? lhsTy
                   : nullptr;
    return reportIfNull(
        constr, ret,
        "Both operands of logical con/disjunction must be boolean constraints");
  } else {
    auto joinTy =
        reportIfNull(constr, lhsTy->join(rhsTy),
                     "The binary operator is not applicable for types " +
                         lhsTy->getName() + " and " + rhsTy->getName());
    if (constr->comparingRelOperator() || constr->equal || constr->unequal) {
      return getOrCreatePrimitive(string("bool"), Type::PrimitiveType::BOOL);
    } else {
      // TODO be more precise here
      return joinTy;
    }
  }
}

shared_ptr<const Type>
ConstraintsTypeChecker::typecheck(CrySLParser::ConstrContext *constr) {
  //  typecheck constraints
  auto sub_constrs = constr->constr();
  if (sub_constrs.size() == 2) {
    if (constr->implies) {
      // both types must be boolean
      auto lhsTy = typecheck(sub_constrs[0]);
      auto rhsTy = typecheck(sub_constrs[1]);
      auto ret =
          lhsTy && rhsTy &&
                  lhsTy->getPrimitiveType() == rhsTy->getPrimitiveType() &&
                  lhsTy->getPrimitiveType() == Type::PrimitiveType::BOOL
              ? lhsTy
              : nullptr;
      return reportIfNull(constr, ret,
                          "Both premisse and conclusion of an implication "
                          "must be boolean constraints");
    } else {
      return typecheckArithCompBinary(constr, sub_constrs[0], sub_constrs[1]);
    }
  } else {
    return typecheckUnary(constr, sub_constrs[0]);
  }
}

bool ConstraintsTypeChecker::typecheck(
    CrySLParser::ConstraintsContext *constr) {
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

bool CrySLTypechecker::CrySLSpec::typecheck(
    CrySLParser::ConstraintsContext *constr) {
  ConstraintsTypeChecker ctc(DefinedObjects);
  return ctc.typecheck(constr);
}
} // namespace CCPP