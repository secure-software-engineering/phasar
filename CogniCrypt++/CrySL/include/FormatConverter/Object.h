#pragma once
#include "Types/Type.h"
#include <CrySLParser.h>
#include <llvm/IR/Instructions.h>
#include <memory>

#include <string>

namespace CCPP {

class Object {
private:
  llvm::Value *val;

public:
  CCPP::Types::Type getType();
  bool operator==(const Object &oc);
};
} // namespace CCPP