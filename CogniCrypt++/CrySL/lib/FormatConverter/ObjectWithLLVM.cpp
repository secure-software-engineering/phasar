#include <FormatConverter/ObjectWithLLVM.h>
#include <iostream>

namespace CCPP {

  ObjectWithLLVM::ObjectWithLLVM(){ this->val = NULL; }

  ObjectWithLLVM::ObjectWithLLVM(llvm::Value *va) { this->val = va; }

  llvm::Type *ObjectWithLLVM::getType() { return val->getType(); }

  bool ObjectWithLLVM::operator==(const ObjectWithLLVM &oc) {
    if(this->val == oc.val)
      return true;

    if (this->type != oc.type)
      return false;

    if (this->type->isIntegerTy()) {
      auto currentObj = llvm::dyn_cast<llvm::ConstantInt>(this->val);
      auto compareObj = llvm::dyn_cast<llvm::ConstantInt>(oc.val);
      if (currentObj!= NULL && compareObj != NULL &&
      currentObj->getValue() == compareObj->getValue())
        return true;
    } else if (this->type->isFloatingPointTy()) {
      auto currentObj = llvm::dyn_cast<llvm::ConstantFP>(this->val);
      auto compareObj = llvm::dyn_cast<llvm::ConstantFP>(oc.val);
      if (currentObj!= NULL && compareObj != NULL &&
      currentObj->getValueAPF().bitwiseIsEqual(compareObj->getValueAPF()))
        return true;
    } else {
      auto currentObj = llvm::dyn_cast<llvm::ConstantDataArray>(this->val);
      auto compareObj = llvm::dyn_cast<llvm::ConstantDataArray>(oc.val);
      if(currentObj!= NULL && compareObj != NULL &&
      currentObj->getElementByteSize() == compareObj->getElementByteSize()  &&
      currentObj->getAsString()== compareObj->getAsString())
        return true;
    }

    return false;
  }

} // namespace CCPP