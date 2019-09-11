#include "CrySLTypechecker.h"
#include <Event.h>
#include <EventConverter.h>
#include <iostream>

namespace CCPP {

Object::Object() { type = getType(); }

Object::Object(CrySLParser::ParamContext *param) {
  if (param->wildCard)
    this->name = "wildCard";
  else if (param->thisPtr)
    this->name = "thisPtr";
  else {
    for (auto paramName : param->memberAccess()->Ident()) {
      this->name = paramName->getText();
    }
  }

  ///////////////////////////////////////////////////////////////
  type = getType();
  ///////////////////////////////////////////////////////////////
}

llvm::Type *Object::getType() { return val->getType(); }

bool Object::operator==(const Object &oc) {
  if (this->type != oc.type)
    return false;

  if (this->type->isIntegerTy()) {
    auto currentObj = llvm::dyn_cast<llvm::ConstantInt>(this->val);
    auto compareObj = llvm::dyn_cast<llvm::ConstantInt>(oc.val);
    if (currentObj->getValue() == compareObj->getValue())
      return true;
  } else if (this->type->isFloatingPointTy()) {
    auto currentObj = llvm::dyn_cast<llvm::ConstantFP>(this->val);
    auto compareObj = llvm::dyn_cast<llvm::ConstantFP>(oc.val);
    if (currentObj->getValueAPF().bitwiseIsEqual(compareObj->getValueAPF()))
      return true;
  } else {
    auto currentObj = llvm::dyn_cast<llvm::ConstantDataArray>(this->val);
    auto compareObj = llvm::dyn_cast<llvm::ConstantDataArray>(oc.val);
    // TO-DO
  }

  return false;
}

} // namespace CCPP