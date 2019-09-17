#pragma once
#include <Parser/CrySLParser.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>
#include <memory>

#include <string>

namespace CCPP {

  class ObjectWithLLVM {
  private:
    llvm::Value *val;
    llvm::Type *type;

  public:
    using TypeT = llvm::Type*;
    ObjectWithLLVM();
    ObjectWithLLVM(llvm::Value *);
    TypeT getType();
    bool operator==(const ObjectWithLLVM &oc);
  };
} // namespace CCPP