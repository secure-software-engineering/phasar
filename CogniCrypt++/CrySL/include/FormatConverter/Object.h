#pragma once
#include "Types/Type.h"
#include <CrySLParser.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>
#include <memory>

#include <string>

namespace CCPP {

class Object {
private:
  llvm::Value *val;
  llvm::Type *type;
  std::string name;
  std::string value;

public:
    using TypeT = llvm::Type*;
	Object(CrySLParser::ParamContext *);
    Object();
	TypeT getType();
	bool operator==(const Object &oc);
};
} // namespace CCPP