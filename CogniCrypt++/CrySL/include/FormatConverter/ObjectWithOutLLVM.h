#pragma once
#include <Parser/CrySLParser.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>
#include <memory>

#include <string>

namespace CCPP {

  class ObjectWithOutLLVM {
  private:
    std::string typeName;

  public:
    ObjectWithOutLLVM(CrySLParser::ParamContext *);
    ObjectWithOutLLVM();
    std::string getType();
    bool operator==(const ObjectWithOutLLVM &oc);
  };
} // namespace CCPP