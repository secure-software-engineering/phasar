#pragma once
#include "Types/Type.h"
#include <CrySLParser.h>
#include <memory>
#include<phasar/PhasarLLVM>

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