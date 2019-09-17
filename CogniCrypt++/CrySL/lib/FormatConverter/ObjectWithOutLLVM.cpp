#include <FormatConverter/ObjectWithOutLLVM.h>
#include <iostream>

namespace CCPP {

  ObjectWithOutLLVM::ObjectWithOutLLVM() { this->typeName = ""; }

  ObjectWithOutLLVM::ObjectWithOutLLVM(CrySLParser::ParamContext *param) {
    if (param->wildCard)
      this->typeName = "wildCard";
    else if (param->thisPtr)
      this->typeName = "thisPtr";
    else {
      for (auto paramName : param->memberAccess()->Ident()) {
        this->typeName = paramName->getText();
      }
    }
  }

  std::string ObjectWithOutLLVM::getType() { return this->typeName; }

  bool ObjectWithOutLLVM::operator==(const ObjectWithOutLLVM &oc) {
    if(this->typeName.compare(oc.typeName))
      return true;
    return false;
  }

} // namespace CCPP