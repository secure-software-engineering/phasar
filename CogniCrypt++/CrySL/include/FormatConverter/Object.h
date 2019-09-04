#pragma once
#include "Types/Type.h"
#include <CrySLParser.h>
#include <llvm/IR/Value.h>
#include<llvm/IR/Value.h>
#include <memory>

#include <string>

namespace CCPP {

class Object {
private:
  llvm::Value *val;
  CCPP::Type::Type *tp;
  std::string name;
  std::string value;

public:
	Object(CrySLParser::ParamContext*);
	std::shared_ptr <CCPP::Types::Type> getType();
	bool operator==(const Object &oc);
};
} // namespace CCPP