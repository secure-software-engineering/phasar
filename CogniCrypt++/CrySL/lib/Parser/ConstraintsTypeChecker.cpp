#include <Parser/CrySLTypechecker.h>
#include <Parser/ErrorHelper.h>
#include <Parser/TokenHelper.h>
#include <Parser/TypeParser.h>
#include <Parser/Types/PointerType.h>
#include <Parser/Types/Type.h>
#include <climits>
#include <cmath>
#include <iostream>

namespace CCPP {
using namespace std;
using namespace Types;
struct ConstraintsTypeChecker {
  const unordered_map<string, shared_ptr<Type>> &DefinedObjects;
  const std::string &filename;
  ConstraintsTypeChecker(
      const unordered_map<string, shared_ptr<Type>> &DefinedObjects,
      const std::string &filename)
      : DefinedObjects(DefinedObjects), filename(filename) {}

  shared_ptr<const Type> typecheck(CrySLParser::ConstrContext *constr);
  shared_ptr<const Type> typecheckUnary(CrySLParser::ConstrContext *constr,
                                        CrySLParser::ConstrContext *sub_constr);
  shared_ptr<const Type>
  typecheckArithCompBinary(CrySLParser::ConstrContext *constr,
                           CrySLParser::ConstrContext *lhs,
                           CrySLParser::ConstrContext *rhs);
  shared_ptr<const Type> typecheckCons(CrySLParser::ConsContext *cons);
  shared_ptr<const Type> typecheckIn(CrySLParser::ArrayElementsContext *elems,
                                     CrySLParser::LitListContext *litList);
  shared_ptr<const Type>
  typecheckLiteralExpr(CrySLParser::LiteralExprContext *litEx);
  shared_ptr<const Type>
  typecheckConsPred(CrySLParser::ConsPredContext *consPred);
  shared_ptr<const Type>
  typecheckArrayElements(CrySLParser::ArrayElementsContext *arrElems);
  shared_ptr<const Type> typecheckLiteral(CrySLParser::LiteralContext *lit);
  shared_ptr<const Type>
  typecheckMemberAccess(CrySLParser::MemberAccessContext *mem);
  shared_ptr<const Type>
  typecheckPredefPred(CrySLParser::PreDefinedPredicateContext *pred);

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
        "The logical negation can only be applied on a boolean value",
        filename);
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
        "Both operands of logical con/disjunction must be boolean constraints",
        filename);
  } else {
    auto joinTy =
        reportIfNull(constr, lhsTy->join(rhsTy),
                     "The binary operator is not applicable for types " +
                         lhsTy->getName() + " and " + rhsTy->getName(),
                     filename);
    if (constr->comparingRelOperator() || constr->equal || constr->unequal) {
      return getOrCreatePrimitive(string("bool"), Type::PrimitiveType::BOOL);
    } else {
      // TODO be more precise here
      return joinTy;
    }
  }
}

shared_ptr<const Type>
ConstraintsTypeChecker::typecheckCons(CrySLParser::ConsContext *cons) {
  if (cons->literalExpr()) {
    return typecheckLiteralExpr(cons->literalExpr());
  } else {
    // arrayElems 'in' '{' litList '}'
    return typecheckIn(cons->arrayElements(), cons->litList());
  }
}
shared_ptr<const Type>
ConstraintsTypeChecker::typecheckIn(CrySLParser::ArrayElementsContext *elems,
                                    CrySLParser::LitListContext *litList) {
  auto elemsTy = typecheckArrayElements(elems);
  auto literals = litList->literal();
  // ignore ellipsis for now ...
  // literals has at least one element
  auto joinTy = typecheckLiteral(literals[0]);
  for (size_t i = 1; i < literals.size(); ++i) {
    auto ty = typecheckLiteral(literals[i]);
    if (joinTy)
      joinTy = joinTy->join(ty);
  }
  if (elemsTy && joinTy && elemsTy->canBeAssignedTo(joinTy.get())) {
    return getOrCreatePrimitive(string("bool"), Type::PrimitiveType::BOOL);
  }
  return nullptr;
}

shared_ptr<const Type> ConstraintsTypeChecker::typecheckConsPred(
    CrySLParser::ConsPredContext *consPred) {
  return typecheckLiteralExpr(consPred->literalExpr());
}
shared_ptr<const Type> ConstraintsTypeChecker::typecheckLiteralExpr(
    CrySLParser::LiteralExprContext *litEx) {
  if (litEx->literal())
    return typecheckLiteral(litEx->literal());
  else if (litEx->memberAccess())
    return typecheckMemberAccess(litEx->memberAccess());
  else // predefined predicate
    return typecheckPredefPred(litEx->preDefinedPredicate());
}
shared_ptr<const Type> typeofInt(antlr4::tree::TerminalNode *intNode) {
  auto intVal = parseInt(intNode->getText());
  // TODO be more precise here
  if (intVal <= INT_MAX) {
    return getOrCreatePrimitive(string("int"), Type::PrimitiveType::INT);
  } else {
    return getOrCreatePrimitive(string("long long"),
                                Type::PrimitiveType::LONGLONG);
  }
}
shared_ptr<const Type>
ConstraintsTypeChecker::typecheckLiteral(CrySLParser::LiteralContext *lit) {
  auto ints = lit->Int();
  if (lit->base) {
    // Int ^ Int
    auto baseVal = parseInt(ints[0]->getText());
    auto expVal = parseInt(ints[1]->getText());
    if (pow(baseVal, expVal) > LONG_LONG_MAX) {
      // std::cerr << Position(lit, filename) << ": Arithmetic overflow at "
      //         << lit->getText() << std::endl;
      reportError(Position(lit, filename),
                  std::string(": Arithmetic overflow at ") + lit->getText());
      return nullptr;
    }
    auto baseTy = typeofInt(ints[0]);
    auto expTy = typeofInt(ints[1]);
    return baseTy->join(expTy);
  } else if (ints.size() == 1) {
    return typeofInt(ints[0]);
  } else if (lit->Bool()) {
    return getOrCreatePrimitive(string("bool"), Type::PrimitiveType::BOOL);
  } else {
    // String
    return createPrimitivePointerType("char", Type::PrimitiveType::CHAR);
  }
}
shared_ptr<const Type> ConstraintsTypeChecker::typecheckMemberAccess(
    CrySLParser::MemberAccessContext *mem) {
  // TODO implement
  auto ident = mem->Ident();
  auto obj = DefinedObjects.find(ident[0]->getText());
  if (obj == DefinedObjects.end()) {
    // std::cerr << "The object '" << ident[0]->getText()
    //          << "' is not defined in the OBJECTS section" << std::endl;
    reportError(Position(mem, filename),
                std::string("The object '") + ident[0]->getText() +
                    "' is not defined in the OBJECTS section");
    return nullptr;
  }
  if (ident.size() == 1) {
    auto ret = obj->second;
    if (mem->deref) {
      if (!ret->isPointerType()) {
        // std::cout << Position(mem, filename)
        //          << ": Dereferencing is only possible on pointers. "
        //          << ident[0]->getText() << " is not a pointer" << std::endl;
        reportError(Position(mem, filename),
                    std::string("Dereferencing is only possible on pointers. " +
                                ident[0]->getText() + " is not a pointer. "));
        return nullptr;
      } else {
        ret = ((Types::PointerType *)ret.get())->getPointerElementType();
      }
    }
    return ret;

  } else {
    // actual member access: Since we don't have the class-definitions, we need
    // to assume, that this is ok (if it is no primitive)
    auto objTy = obj->second;
    if (mem->dot && objTy->isPointerType()) {
      // std::cerr << "Member-access using the dot (.) is not allowed on
      // pointers."
      //          << ident[0]->getText() << " is a pointer" << std::endl;
      reportError(
          Position(mem, filename),
          {"Member-access using the dot (.) is not allowed on pointers.",
           ident[0]->getText(), " is a pointer"});
      return nullptr;
    } else if (mem->arrow && !objTy->isPointerType()) {
      // std::cerr
      //    << "Member-access using the arrow (->) is only allowed on pointers."
      //    << ident[0]->getText() << " is not a pointer" << std::endl;
      reportError(
          Position(mem, filename),
          {"Member-access using the arrow (->) is only allowed on pointers.",
           ident[0]->getText(), " is not a pointer"});
      return nullptr;
    }
    return getOrCreatePrimitive(string("<top>"), Type::PrimitiveType::TOP);
  }
}
shared_ptr<const Type> ConstraintsTypeChecker::typecheckPredefPred(
    CrySLParser::PreDefinedPredicateContext *pred) {
  // TODO implement
  return getOrCreatePrimitive(string("bool"), Type::PrimitiveType::BOOL);
}
shared_ptr<const Type> ConstraintsTypeChecker::typecheckArrayElements(
    CrySLParser::ArrayElementsContext *arrElems) {
  if (arrElems->el) {
    auto arrTy = typecheckConsPred(arrElems->consPred());
    if (arrTy && arrTy->isPointerType()) {
      return ((PointerType *)arrTy.get())->getPointerElementType();
    }
    return nullptr;
  } else {
    return typecheckConsPred(arrElems->consPred());
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
                          "must be boolean constraints",
                          filename);
    } else {
      return typecheckArithCompBinary(constr, sub_constrs[0], sub_constrs[1]);
    }
  } else if (sub_constrs.size() == 1) {
    // not (!)
    return typecheckUnary(constr, sub_constrs[0]);
  } else {
    // cons
    return typecheckCons(constr->cons());
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
        // cerr << Position(constr, filename) << ": The constraint is not
        // boolean"
        //     << endl;
        reportError(Position(constr, filename),
                    "The constraint is not boolean");
      }
    }
  }
  return succ;
}

bool CrySLTypechecker::CrySLSpec::typecheck(
    CrySLParser::ConstraintsContext *constr) {
  ConstraintsTypeChecker ctc(DefinedObjects, filename);
  return ctc.typecheck(constr);
}
} // namespace CCPP